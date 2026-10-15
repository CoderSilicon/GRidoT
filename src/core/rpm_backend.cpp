#include "../include/rpm_backend.h"

#include <algorithm>
#include <filesystem>
#include <sstream>

#include "../include/util.h"

namespace
{

constexpr char kFieldSep = '\x1f';
constexpr char kGroupSep = '\x1d';
constexpr char kRecSep = '\x1e';

const std::set<std::string> kInstallOnly = {
    "kernel",
    "kernel-PAE",
    "installonlypkg(kernel)",
    "installonlypkg(kernel-module)",
    "installonlypkg(vm)",
    "multiversion(kernel)",
};

std::string query_format(bool with_recommends)
{
  std::ostringstream f;
  f << "%{NAME}" << kFieldSep << "%{ARCH}" << kFieldSep << "%{EPOCH}" << kFieldSep
    << "%{VERSION}-%{RELEASE}" << kFieldSep << "%{GROUP}" << kFieldSep << "%{SIZE}" << kGroupSep
    << "[%{REQUIRENAME}" << kFieldSep << "]" << kGroupSep << "[%{PROVIDENAME}" << kFieldSep << "]"
    << kGroupSep << "[%{FILENAMES}" << kFieldSep << "]" << kGroupSep;
  if (with_recommends)
    f << "[%{RECOMMENDNAME}" << kFieldSep << "]";
  f << kGroupSep << kRecSep;
  return f.str();
}

std::vector<std::string> array_field(const std::string& field)
{
  std::vector<std::string> out;
  for (const std::string& s : util::split(field, kFieldSep))
  {
    if (!s.empty())
      out.push_back(s);
  }
  return out;
}

bool usable(const util::ProcResult& r)
{
  return r.spawned && r.exit_code == 0 && !r.out.empty() && !util::contains(r.out, "%{");
}

std::string normalize_reason(const std::string& raw)
{
  std::string s = util::to_lower(raw);
  s.erase(std::remove_if(s.begin(), s.end(),
                         [](char ch)
                         {
                           return ch == ' ' || ch == '-' || ch == '_';
                         }),
          s.end());
  return s;
}

void read_list_config(const std::string& path, const std::string& key, std::set<std::string>& out)
{
  std::string data;
  if (!util::read_file(path, data))
    return;
  for (const std::string& raw : util::split(data, '\n'))
  {
    const std::string line = util::trim(raw);
    if (line.empty() || line[0] == '#' || line[0] == ';')
      continue;
    const size_t eq = line.find('=');
    if (eq == std::string::npos || util::trim(line.substr(0, eq)) != key)
      continue;
    for (const std::string& value : util::split(util::trim(line.substr(eq + 1)), ','))
    {
      std::istringstream ss(value);
      std::string token;
      while (ss >> token)
      {
        if (!util::contains(token, "*"))
          out.insert(token);
      }
    }
  }
}

void read_list_dir(const std::string& dir, std::set<std::string>& out)
{
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec))
    return;
  std::error_code iter_ec;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir, iter_ec))
  {
    if (!entry.is_regular_file())
      continue;
    std::string data;
    if (!util::read_file(entry.path().string(), data))
      continue;
    for (const std::string& raw : util::split(data, '\n'))
    {
      std::string line = util::trim(raw);
      if (const size_t hash = line.find('#'); hash != std::string::npos)
        line = util::trim(line.substr(0, hash));
      if (!line.empty() && !util::contains(line, "*"))
        out.insert(line);
    }
  }
}

} // namespace

std::string RpmBackend::name() const
{
  if (util::have_cmd("dnf"))
    return "dnf";
  if (util::have_cmd("yum"))
    return "yum";
  return "rpm";
}

bool RpmBackend::available() const
{
  if (!util::have_cmd("rpm"))
    return false;
  return util::have_cmd("dnf") || util::have_cmd("yum") ||
         util::file_exists("/var/lib/dnf/yumdb") || util::file_exists("/var/lib/yum/yumdb");
}

std::string RpmBackend::removal_hint() const
{
  const std::string pm = name();
  if (pm == "rpm")
    return "sudo rpm -e <pkg>";
  return "sudo " + pm + " autoremove or sudo " + pm + " remove <pkg>";
}

bool RpmBackend::load(std::string& err)
{
  if (!load_packages(err))
    return false;
  if (!load_install_reasons(err))
    return false;
  load_protections();
  return true;
}

bool RpmBackend::load_packages(std::string& err)
{
  const std::string prefix = "LC_ALL=C rpm -qa --qf '";
  util::ProcResult r = util::run(prefix + query_format(true) + "'");
  if (!usable(r))
    r = util::run(prefix + query_format(false) + "'");
  if (!usable(r))
  {
    err = "failed to read the rpm database";
    return false;
  }

  const std::string native = util::trim(util::run("uname -m").out);

  for (const std::string& rec : util::split(r.out, kRecSep))
  {
    if (util::trim(rec).empty())
      continue;
    const std::vector<std::string> g = util::split(rec, kGroupSep);
    if (g.size() < 4)
      continue;
    const std::vector<std::string> f = util::split(g[0], kFieldSep);
    if (f.size() < 6 || f[0].empty())
      continue;

    Pkg p;
    p.name = f[0];
    if (p.name == "gpg-pubkey")
      continue;
    p.arch = f[1];
    const std::string& epoch = f[2];
    const bool has_epoch = !epoch.empty() && epoch != "(none)" && epoch != "0";
    p.version = (has_epoch ? epoch + ":" : "") + f[3];
    p.section = f[4];
    p.installed = true;
    try
    {
      p.size_kb = (std::stoll(util::trim(f[5])) + 1023) / 1024;
    }
    catch (...)
    {
      p.size_kb = 0;
    }
    if (g.size() > 1)
      p.depends = array_field(g[1]);
    if (g.size() > 2)
      p.provides = array_field(g[2]);
    if (g.size() > 3)
      p.file_provides = array_field(g[3]);
    if (g.size() > 4)
      p.recommends = array_field(g[4]);

    const std::string nvra = p.name + "-" + f[3] + "." + p.arch;
    nvra_to_name[nvra] = p.name;
    if (has_epoch)
      nvra_to_name[p.name + "-" + epoch + ":" + f[3] + "." + p.arch] = p.name;

    auto it = packages.find(p.name);
    if (it != packages.end())
    {
      if (it->second.arch == p.arch)
      {
        if (it->second.version != p.version)
          multiversion.insert(p.name);
        continue;
      }
      if (!native.empty() && it->second.arch != native && p.arch == native)
        it->second = std::move(p);
      continue;
    }
    packages.emplace(p.name, std::move(p));
  }

  if (packages.empty())
  {
    err = "no usable package records parsed from rpm";
    return false;
  }
  return true;
}

bool RpmBackend::load_install_reasons(std::string& err)
{
  if (util::have_cmd("dnf"))
  {
    util::ProcResult r = util::run(
        "LC_ALL=C dnf -q --disablerepo='*' repoquery --installed "
        "--queryformat '%{name}\t%{reason}\n'");
    if (usable(r))
    {
      size_t reasoned = 0;
      for (const std::string& line : util::split(r.out, '\n'))
      {
        const size_t tab = line.find('\t');
        if (tab == std::string::npos)
          continue;
        const std::string pkg_name = util::trim(line.substr(0, tab));
        const std::string reason = util::trim(line.substr(tab + 1));
        if (pkg_name.empty() || reason.empty())
          continue;
        auto it = packages.find(pkg_name);
        if (it == packages.end())
          continue;
        reasoned++;
        const std::string norm = normalize_reason(reason);
        it->second.auto_installed = norm == "dependency" || norm == "weakdependency";
      }
      if (reasoned == packages.size())
        return true;
      for (auto& kv : packages)
        kv.second.auto_installed = false;
    }

    util::ProcResult u = util::run(
        "LC_ALL=C dnf -q --disablerepo='*' repoquery --userinstalled "
        "--queryformat '%{name}\n'");
    if (usable(u))
    {
      std::set<std::string> keep;
      for (const std::string& line : util::split(u.out, '\n'))
      {
        const std::string n = util::trim(line);
        if (!n.empty())
          keep.insert(n);
      }
      if (!keep.empty())
      {
        for (auto& kv : packages)
          kv.second.auto_installed = keep.count(kv.first) == 0;
        return true;
      }
    }
  }

  if (load_yumdb_reasons())
    return true;

  err = "cannot determine install reasons (found no dnf/yum state to read)";
  return false;
}

bool RpmBackend::load_yumdb_reasons()
{
  bool any = false;
  for (const char* root : {"/var/lib/dnf/yumdb", "/var/lib/yum/yumdb"})
  {
    std::error_code dir_ec;
    if (!std::filesystem::is_directory(root, dir_ec))
      continue;
    std::error_code repo_ec;
    for (const std::filesystem::directory_entry& repo :
         std::filesystem::directory_iterator(root, repo_ec))
    {
      if (!repo.is_directory())
        continue;
      std::error_code pkg_ec;
      for (const std::filesystem::directory_entry& entry :
           std::filesystem::directory_iterator(repo.path(), pkg_ec))
      {
        if (!entry.is_directory())
          continue;
        const auto name_it = nvra_to_name.find(entry.path().filename().string());
        if (name_it == nvra_to_name.end())
          continue;
        const auto pkg_it = packages.find(name_it->second);
        if (pkg_it == packages.end())
          continue;
        std::string reason;
        util::read_file(entry.path().string() + "/reason", reason);
        reason = util::trim(reason);
        const bool keep =
            reason.empty()
                ? util::file_exists(entry.path().string() + "/userinstalled")
                : normalize_reason(reason) != "dep";
        if (!keep)
          pkg_it->second.auto_installed = true;
        any = true;
      }
    }
  }
  return any;
}

void RpmBackend::load_protections()
{
  std::set<std::string> installonly = kInstallOnly;
  std::set<std::string> protected_pkgs = {"dnf", "dnf5"};
  for (const char* cfg : {"/etc/dnf/dnf.conf", "/etc/yum/yum.conf"})
  {
    read_list_config(cfg, "installonlypkgs", installonly);
    read_list_config(cfg, "protected_packages", protected_pkgs);
  }
  read_list_dir("/etc/dnf/protected.d", protected_pkgs);
  read_list_dir("/etc/yum/protected.d", protected_pkgs);

  for (auto& [name, p] : packages)
  {
    if (multiversion.count(name) || installonly.count(name) || protected_pkgs.count(name))
    {
      p.essential = true;
      continue;
    }
    const auto provides_it =
        std::find_if(p.provides.begin(), p.provides.end(),
                     [&installonly](const std::string& s) { return installonly.count(s) > 0; });
    if (provides_it != p.provides.end())
      p.essential = true;
  }
}
