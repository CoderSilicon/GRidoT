#include "../include/apt_backend.h"

#include <algorithm>
#include <set>
#include <sstream>

#include "../include/util.h"

namespace
{

constexpr char kFieldSep = '\x1f';
constexpr char kRecSep = '\x1e';

std::string strip_constraint(const std::string& tok)
{
  std::string t = tok;
  if (const size_t lt = t.find('<'); lt != std::string::npos)
    t = t.substr(0, lt);
  if (const size_t lp = t.find('('); lp != std::string::npos)
    t = t.substr(0, lp);
  t = util::trim(t);
  if (const size_t colon = t.rfind(':');
      colon != std::string::npos && t.find(':', colon + 1) == std::string::npos)
  {
    const std::string arch = t.substr(colon + 1);
    if (arch != "any")
      t = t.substr(0, colon);
  }
  return util::trim(t);
}

} // namespace

bool AptBackend::available() const
{
  return util::have_cmd("dpkg-query");
}

std::vector<std::string> AptBackend::parse_relation(const std::string& raw)
{
  std::vector<std::string> names;
  for (const std::string& clause : util::split(raw, ','))
  {
    for (const std::string& alt : util::split(clause, '|'))
    {
      const std::string n = strip_constraint(alt);
      if (!n.empty())
        names.push_back(n);
    }
  }
  return names;
}

bool AptBackend::load(std::string& err)
{
  std::ostringstream fmt;
  fmt << "${Package}" << kFieldSep << "${Architecture}" << kFieldSep << "${Version}" << kFieldSep
      << "${Section}" << kFieldSep << "${Priority}" << kFieldSep << "${Essential}" << kFieldSep
      << "${Status}" << kFieldSep << "${Installed-Size}" << kFieldSep << "${Pre-Depends}"
      << kFieldSep << "${Depends}" << kFieldSep << "${Recommends}" << kFieldSep << "${Provides}"
      << kRecSep;

  const std::string cmd = "LC_ALL=C dpkg-query -W -f='" + fmt.str() + "' 2>/dev/null";
  util::ProcResult r = util::run(cmd);
  if (!r.spawned)
  {
    err = "failed to execute dpkg-query";
    return false;
  }
  if (r.out.empty())
  {
    err = "dpkg-query returned no packages";
    return false;
  }

  // apt may append a warning line; keep only records we can parse.
  std::set<std::string> seen_arch;
  for (const std::string& rec : util::split(r.out, kRecSep))
  {
    if (util::trim(rec).empty())
      continue;
    std::vector<std::string> f = util::split(rec, kFieldSep);
    if (f.size() < 12)
      continue;
    if (f[0].empty())
      continue;

    const std::string& name = f[0];
    const std::string& arch = f[1];
    const std::string dedup = name + ":" + arch;

    Pkg p;
    p.name = name;
    p.version = f[2];
    p.section = f[3];
    p.priority = f[4];
    p.essential = util::trim(f[5]) == "yes";

    const std::vector<std::string> st = util::split(f[6], ' ');
    p.installed = st.size() >= 3 && st[0] == "install" && st[1] == "ok" && st[2] == "installed";

    try
    {
      p.size_kb = std::stoll(util::trim(f[7]));
    }
    catch (...)
    {
      p.size_kb = 0;
    }

    p.pre_depends = parse_relation(f[8]);
    p.depends = parse_relation(f[9]);
    p.recommends = parse_relation(f[10]);
    p.provides = parse_relation(f[11]);

    // Unqualified names are the dependency-graph identity. If several
    // architectures of the same name are installed, keep the native one.
    if (!seen_arch.insert(dedup).second)
      continue;
    auto existing = packages.find(name);
    if (existing != packages.end())
    {
      if (existing->second.arch == "amd64" && p.arch != "amd64")
        continue;
      existing->second = std::move(p);
      continue;
    }
    packages.emplace(name, std::move(p));
  }

  if (packages.empty())
  {
    err = "no usable package records parsed from dpkg-query";
    return false;
  }

  if (!load_auto_flags())
  {
    err = "could not read auto-install state from apt";
    return false;
  }

  load_install_origins();
  return true;
}

bool AptBackend::load_install_origins()
{
  std::string data;
  if (!util::read_file("/var/log/apt/history.log", data))
    return false;

  std::vector<std::string> lines;
  for (const std::string& raw : util::split(data, '\n'))
  {
    if (!lines.empty() && !raw.empty() && (raw[0] == ' ' || raw[0] == '\t'))
    {
      lines.back() += ' ';
      lines.back() += util::trim(raw);
    }
    else
    {
      lines.push_back(util::trim(raw));
    }
  }

  struct Stanza
  {
    std::vector<std::string> requested;
    std::vector<std::pair<std::string, bool>> touched; // name, automatic
  };
  Stanza cur;
  bool any = false;

  auto flush = [&]
  {
    if (any && !cur.requested.empty())
    {
      std::string culprit = util::join(cur.requested, ", ");
      for (const auto& [name, automatic] : cur.touched)
      {
        if (!automatic)
          continue;
        install_origin[name] = culprit;
      }
    }
    cur = Stanza{};
    any = false;
  };

  auto split_top = [](const std::string& v)
  {
    std::vector<std::string> out;
    int depth = 0;
    std::string cur;
    for (char ch : v)
    {
      if (ch == '(')
        depth++;
      if (ch == ')')
        depth--;
      if (ch == ',' && depth <= 0)
      {
        out.push_back(cur);
        cur.clear();
      }
      else
      {
        cur += ch;
      }
    }
    out.push_back(cur);
    return out;
  };

  auto parse_field =
      [&split_top](const std::string& value, std::vector<std::pair<std::string, bool>>& out)
  {
    for (const std::string& entry : split_top(value))
    {
      const std::string e = util::trim(entry);
      if (e.empty())
        continue;
      const size_t paren = e.find('(');
      std::string name = paren == std::string::npos ? e : util::trim(e.substr(0, paren));
      if (const size_t colon = name.rfind(':'); colon != std::string::npos)
      {
        name = name.substr(0, colon);
      }
      const bool automatic = util::contains(e, "automatic");
      if (!name.empty())
        out.emplace_back(name, automatic);
    }
  };

  for (const std::string& line : lines)
  {
    if (line.empty())
    {
      flush();
      continue;
    }
    const size_t colon = line.find(':');
    if (colon == std::string::npos)
      continue;
    const std::string key = line.substr(0, colon);
    const std::string value = util::trim(line.substr(colon + 1));
    if (key == "Install" || key == "Upgrade")
    {
      any = true;
      std::vector<std::pair<std::string, bool>> entries;
      parse_field(value, entries);
      for (const auto& e : entries)
      {
        cur.touched.push_back(e);
        if (!e.second)
          cur.requested.push_back(e.first);
      }
    }
  }
  flush();
  return !install_origin.empty();
}

bool AptBackend::load_auto_flags()
{
  auto mark = [&](const std::string& list, bool flag)
  {
    for (const std::string& line : util::split(list, '\n'))
    {
      const std::string n = util::trim(line);
      if (n.empty())
        continue;
      auto it = packages.find(n);
      if (it != packages.end())
        it->second.auto_installed = flag;
    }
  };

  // Preferred source: apt's own extended state file.
  if (std::string data; util::read_file("/var/lib/apt/extended_states", data))
  {
    std::set<std::string> autos;
    std::string cur;
    for (const std::string& line : util::split(data, '\n'))
    {
      const std::string t = util::trim(line);
      if (t.empty())
      {
        cur.clear();
        continue;
      }
      if (util::starts_with(t, "Package:"))
      {
        cur = util::trim(t.substr(8));
      }
      else if (util::starts_with(t, "Auto-Installed:") && util::trim(t.substr(15)) == "1")
      {
        if (!cur.empty())
          autos.insert(cur);
      }
    }
    if (!autos.empty())
    {
      for (const auto& kv : packages)
      {
        if (autos.count(kv.first))
          packages[kv.first].auto_installed = true;
      }
      return true;
    }
  }

  // Fallback: ask apt directly.
  util::ProcResult auto_r = util::run("LC_ALL=C apt-mark showauto");
  if (auto_r.spawned && auto_r.exit_code == 0)
  {
    mark(auto_r.out, true);
    return true;
  }
  util::ProcResult man_r = util::run("LC_ALL=C apt-mark showmanual");
  if (man_r.spawned && man_r.exit_code == 0)
  {
    for (auto& kv : packages)
      kv.second.auto_installed = false;
    std::set<std::string> manual;
    for (const std::string& line : util::split(man_r.out, '\n'))
    {
      const std::string n = util::trim(line);
      if (!n.empty())
        manual.insert(n);
    }
    for (auto& kv : packages)
    {
      if (!manual.count(kv.first))
        kv.second.auto_installed = true;
    }
    return true;
  }
  return false;
}
