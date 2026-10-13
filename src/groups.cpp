#include "include/groups.h"

#include <algorithm>
#include <sstream>

#include "include/graph.h"
#include "include/util.h"

namespace
{

constexpr const char* kHeader = "# gridot: package tracker\n";

} // namespace

std::string GroupStore::path()
{
  return util::config_dir() + "/groups.tsv";
}

bool GroupStore::load(std::string& err)
{
  groups.clear();
  std::string data;
  if (!util::read_file(path(), data))
    return true;
  for (const std::string& raw : util::split(data, '\n'))
  {
    const std::string line = util::trim(raw);
    if (line.empty() || line[0] == '#')
      continue;
    const std::vector<std::string> f = util::split(line, '\t');
    if (f.size() < 2)
      continue;
    Group* g = find(f[0]);
    if (!g)
    {
      groups.push_back(Group{f[0], {}});
      g = &groups.back();
    }
    const std::string member = util::trim(f[1]);
    if (!member.empty() &&
        std::find(g->members.begin(), g->members.end(), member) == g->members.end())
    {
      g->members.push_back(member);
    }
  }
  (void)err;
  return true;
}

bool GroupStore::save(std::string& err) const
{
  const std::string dir = util::config_dir();
  if (!util::make_dirs(dir))
  {
    err = "cannot create " + dir;
    return false;
  }
  std::ostringstream os;
  os << kHeader;
  for (const Group& g : groups)
  {
    for (const std::string& m : g.members)
      os << g.name << '\t' << m << '\n';
  }
  if (!util::write_file(path(), os.str()))
  {
    err = "cannot write " + path();
    return false;
  }
  return true;
}

Group* GroupStore::find(const std::string& name)
{
  for (Group& g : groups)
    if (g.name == name)
      return &g;
  return nullptr;
}

const Group* GroupStore::find(const std::string& name) const
{
  for (const Group& g : groups)
    if (g.name == name)
      return &g;
  return nullptr;
}

void GroupStore::add(const std::string& name, const std::vector<std::string>& members)
{
  Group* g = find(name);
  if (!g)
  {
    groups.push_back(Group{name, {}});
    g = &groups.back();
  }
  for (const std::string& m : members)
  {
    if (std::find(g->members.begin(), g->members.end(), m) == g->members.end())
    {
      g->members.push_back(m);
    }
  }
}

bool GroupStore::remove(const std::string& name)
{
  const auto it =
      std::remove_if(groups.begin(), groups.end(), [&](const Group& g) { return g.name == name; });
  const bool removed = it != groups.end();
  groups.erase(it, groups.end());
  return removed;
}

std::vector<std::string> GroupStore::groups_containing(const std::string& pkg) const
{
  std::vector<std::string> out;
  for (const Group& g : groups)
  {
    if (std::find(g.members.begin(), g.members.end(), pkg) != g.members.end())
    {
      out.push_back(g.name);
    }
  }
  return out;
}

std::set<std::string> GroupStore::all_members() const
{
  std::set<std::string> out;
  for (const Group& g : groups)
    out.insert(g.members.begin(), g.members.end());
  return out;
}

std::set<std::string> GroupStore::names() const
{
  std::set<std::string> out;
  for (const Group& g : groups)
    out.insert(g.name);
  return out;
}

std::vector<GroupFinding> audit_groups(const GroupStore& store, const Backend& backend)
{
  std::vector<GroupFinding> out;

  std::map<std::string, std::set<std::string>> rdeps;
  for (const auto& [name, p] : backend.packages)
  {
    if (!p.installed)
      continue;
    std::set<std::string> rels;
    rels.insert(p.pre_depends.begin(), p.pre_depends.end());
    rels.insert(p.depends.begin(), p.depends.end());
    for (const std::string& r : rels)
    {
      auto it = backend.packages.find(r);
      if (it != backend.packages.end() && it->second.installed)
      {
        rdeps[r].insert(name);
      }
    }
  }
  // Resolve virtual names to real providers for the reverse edges too.
  std::map<std::string, std::set<std::string>> provides;
  for (const auto& [name, p] : backend.packages)
  {
    if (!p.installed)
      continue;
    for (const std::string& v : p.provides)
      provides[v].insert(name);
  }
  for (const auto& [name, p] : backend.packages)
  {
    if (!p.installed)
      continue;
    std::set<std::string> rels;
    rels.insert(p.pre_depends.begin(), p.pre_depends.end());
    rels.insert(p.depends.begin(), p.depends.end());
    for (const std::string& r : rels)
    {
      if (backend.packages.count(r))
        continue;
      auto pit = provides.find(r);
      if (pit == provides.end())
        continue;
      for (const std::string& real : pit->second)
        rdeps[real].insert(name);
    }
  }

  for (const Group& g : store.groups)
  {
    const std::set<std::string> members(g.members.begin(), g.members.end());

    // A group is "broken" when a member is no longer installed. That is the
    // signal the whole rest of the group may now be pointless.
    std::vector<std::string> missing;
    for (const std::string& m : members)
    {
      const Pkg* p = backend.get(m);
      if (!p || !p->installed)
        missing.push_back(m);
    }
    const bool group_broken = !missing.empty();

    for (const std::string& m : members)
    {
      const Pkg* p = backend.get(m);
      GroupFinding f;
      f.group = g.name;
      f.member = m;
      f.member_installed = p && p->installed;
      f.size_kb = p ? p->size_kb : 0;

      if (!f.member_installed)
      {
        f.reason = "not installed (removed from this system)";
        out.push_back(std::move(f));
        continue;
      }

      // Who wants this outside the group?
      auto rit = rdeps.find(m);
      if (rit != rdeps.end())
      {
        for (const std::string& d : rit->second)
        {
          if (members.count(d))
            continue;
          f.external_dependents.push_back(d);
        }
      }
      f.externally_needed = !f.external_dependents.empty();

      if (!f.externally_needed)
      {
        // Nothing outside the group needs it. If the group is intact and this
        // package is what the user deliberately asked for, it is still fine.
        if (group_broken)
        {
          f.at_risk = true;
          std::string lost;
          if (missing.size() == 1)
          {
            lost = "lost " + missing.front();
          }
          else
          {
            lost = "lost " + std::to_string(missing.size()) + " members (" +
                   util::join(missing, ", ") + ")";
          }
          f.reason = "group '" + g.name + "' " + lost + "; nothing else needs this";
        }
        else
        {
          f.reason = "kept only by group '" + g.name + "' (intentional)";
        }
      }
      else
      {
        f.reason = "still required by " + util::join(f.external_dependents, ", ");
      }
      out.push_back(std::move(f));
    }
  }

  std::sort(out.begin(), out.end(),
            [](const GroupFinding& a, const GroupFinding& b)
            {
              if (a.at_risk != b.at_risk)
                return a.at_risk;
              if (a.size_kb != b.size_kb)
                return a.size_kb > b.size_kb;
              return a.member < b.member;
            });
  return out;
}
