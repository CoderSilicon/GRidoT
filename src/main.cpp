#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "include/backend.h"
#include "include/graph.h"
#include "include/groups.h"
#include "include/report.h"
#include "include/util.h"

namespace
{

constexpr const char* kVersion = "0.1.0";

void usage()
{
  std::cout <<
      R"(GRidoT: Get Rid of That - find packages you no longer need

USAGE:
  gridot [scan] [options]        report orphaned packages by section
  gridot track <name> <pkg>...   group a use case so its parts are tracked
  gridot untrack <name>         stop tracking a group
  gridot groups                  list tracked groups

SCAN OPTIONS:
  -m, --min-size <MiB>   only report orphans at least this large
  -n, --limit <N>        show at most N orphans in the detail list
  -r, --root <pkg>       treat pkg as a manual root (repeatable)
      --json             machine-readable output
      --no-groups        skip the tracked-groups section
      --no-recommends    ignore Recommends edges (over-reports, apt installs them)

TRACK OPTIONS:
  gridot track <name> <pkg>...     create a group
  gridot track -g <name> <pkg>...  add packages to an existing group
  Members do not have to be installed; a group with a missing member is
  reported as BROKEN and its remaining members as AT RISK.

gridot never installs or removes anything. It only reads package state and reports it.
)";
}

struct Options
{
  std::string command = "scan";
  std::string group_name;
  std::vector<std::string> positional;
  std::vector<std::string> roots;
  long long min_kb = 0;
  size_t limit = 0;
  bool json = false;
  bool no_groups = false;
  bool no_recommends = false;
};

bool parse_args(int argc, char** argv, Options& o, std::string& err)
{
  if (argc > 1)
  {
    const std::string first = argv[1];
    if (!first.empty() && first[0] != '-')
    {
      o.command = first;
      for (int i = 2; i < argc; i++)
        o.positional.push_back(argv[i]);
    }
    else
    {
      for (int i = 1; i < argc; i++)
        o.positional.push_back(argv[i]);
    }
  }

  std::vector<std::string> args = o.positional;
  o.positional.clear();
  for (size_t i = 0; i < args.size(); i++)
  {
    const std::string& a = args[i];
    auto next = [&](std::string& dst)
    {
      if (i + 1 >= args.size())
        return false;
      dst = args[++i];
      return true;
    };

    if (a == "-h" || a == "--help")
    {
      o.command = "help";
      return true;
    }
    else if (a == "-V" || a == "--version")
    {
      o.command = "version";
      return true;
    }
    else if (a == "-m" || a == "--min-size")
    {
      std::string v;
      if (!next(v))
      {
        err = a + " needs a value";
        return false;
      }
      o.min_kb = std::stoll(v) * 1024;
    }
    else if (a == "-n" || a == "--limit")
    {
      std::string v;
      if (!next(v))
      {
        err = a + " needs a value";
        return false;
      }
      o.limit = static_cast<size_t>(std::stoull(v));
    }
    else if (a == "--json")
    {
      o.json = true;
    }
    else if (a == "--no-groups")
    {
      o.no_groups = true;
    }
    else if (a == "--no-recommends")
    {
      o.no_recommends = true;
    }
    else if (a == "-g" || a == "--group")
    {
      if (!next(o.group_name))
      {
        err = a + " needs a value";
        return false;
      }
    }
    else if (a == "-r" || a == "--root")
    {
      std::string v;
      if (!next(v))
      {
        err = a + " needs a value";
        return false;
      }
      o.roots.push_back(v);
    }
    else if (!a.empty() && a[0] == '-' && a != "-")
    {
      err = "unknown option: " + a;
      return false;
    }
    else
    {
      o.positional.push_back(a);
    }
  }
  return true;
}

std::string json_escape(const std::string& s)
{
  std::string out;
  for (char ch : s)
  {
    switch (ch)
    {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += ch;
    }
  }
  return out;
}

int cmd_scan(const Options& o)
{
  auto backend = detect_backend();
  if (!backend)
  {
    std::cerr << "gridot: no supported package manager found (looked for dpkg/apt)\n";
    return 2;
  }
  std::string err;
  if (!backend->load(err))
  {
    std::cerr << "gridot: " << err << "\n";
    return 2;
  }

  GroupStore store;
  if (!store.load(err))
  {
    std::cerr << "gridot: " << err << "\n";
    return 2;
  }

  std::set<std::string> roots = store.all_members();
  roots.insert(o.roots.begin(), o.roots.end());

  for (const std::string& r : o.roots)
  {
    const Pkg* p = backend->get(r);
    if (!p || !p->installed)
      std::cerr << "gridot: warning: " << r << " is not installed\n";
  }

  const GraphResult graph = analyze(*backend, roots, !o.no_recommends);
  const std::vector<GroupFinding> findings =
      o.no_groups ? std::vector<GroupFinding>() : audit_groups(store, *backend);

  if (o.json)
  {
    std::cout << "{\n";
    std::cout << "  \"backend\": \"" << json_escape(backend->name()) << "\",\n";
    std::cout << "  \"installed\": " << graph.installed_count << ",\n";
    std::cout << "  \"reachable\": " << graph.reachable_count << ",\n";
    std::cout << "  \"reclaimable_kb\": " << graph.total_orphan_kb << ",\n";
    std::cout << "  \"orphans\": [";
    size_t shown = 0;
    for (const Orphan& op : graph.orphans)
    {
      if (o.min_kb && op.size_kb < o.min_kb)
        continue;
      if (o.limit && shown >= o.limit)
        break;
      if (shown)
        std::cout << ",";
      std::cout << "\n    {\"name\": \"" << json_escape(op.name)
                << "\", \"size_kb\": " << op.size_kb << ", \"section\": \""
                << json_escape(op.section) << "\", \"pulled_in_by\": \""
                << json_escape(op.pulled_in_by)
                << "\", \"auto\": " << (op.auto_installed ? "true" : "false") << "}";
      shown++;
    }
    std::cout << (shown ? "\n  ]\n}\n" : "]\n}\n");
    return 0;
  }

  if (o.limit)
  {
    GraphResult trimmed = graph;
    trimmed.orphans.resize(std::min(o.limit, graph.orphans.size()));
    std::cout << render_report(*backend, trimmed, store, findings, roots);
    return 0;
  }
  std::cout << render_report(*backend, graph, store, findings, roots);
  return 0;
}

int cmd_track(const Options& o)
{
  std::string gname = o.group_name;
  std::vector<std::string> members;
  for (const std::string& a : o.positional)
  {
    if (gname.empty())
    {
      gname = a;
    }
    else
    {
      members.push_back(a);
    }
  }
  if (gname.empty())
  {
    std::cerr << "gridot: usage: gridot track <name> <pkg>...\n"
                 "       or: gridot track -g <name> <pkg>...\n";
    return 2;
  }
  if (members.empty())
  {
    std::cerr << "gridot: no packages given for group '" << gname << "'\n";
    return 2;
  }

  auto backend = detect_backend();
  if (!backend)
    return 2;
  std::string err;
  if (!backend->load(err))
  {
    std::cerr << "gridot: " << err << "\n";
    return 2;
  }

  GroupStore store;
  store.load(err);

  // Recording a member that is not installed right now is allowed and useful:
  // it is exactly how a group is later recognised as broken.
  std::vector<std::string> unknown, absent;
  for (const std::string& m : members)
  {
    const Pkg* p = backend->get(m);
    if (!p)
    {
      unknown.push_back(m);
    }
    else if (!p->installed)
    {
      absent.push_back(m);
    }
  }
  if (!unknown.empty())
  {
    std::cerr << "gridot: no such package: " << util::join(unknown, ", ") << "\n";
    return 1;
  }
  if (!absent.empty())
  {
    std::cerr << "gridot: note: not currently installed, tracking anyway: "
              << util::join(absent, ", ") << "\n";
  }

  store.add(gname, members);
  if (!store.save(err))
  {
    std::cerr << "gridot: " << err << "\n";
    return 1;
  }

  std::cout << "tracked group '" << gname << "': " << util::join(members, " ") << "\n";
  std::cout << "store: " << GroupStore::path() << "\n";
  std::cout << "run 'gridot scan' to see what this keeps alive.\n";
  return 0;
}

int cmd_untrack(const Options& o)
{
  std::string gname = o.group_name;
  for (const std::string& a : o.positional)
  {
    if (gname.empty())
      gname = a;
  }
  if (gname.empty())
  {
    std::cerr << "gridot: usage: gridot untrack <name>\n";
    return 2;
  }
  GroupStore store;
  std::string err;
  store.load(err);
  if (!store.remove(gname))
  {
    std::cerr << "gridot: no such group: " << gname << "\n";
    return 1;
  }
  if (!store.save(err))
  {
    std::cerr << "gridot: " << err << "\n";
    return 1;
  }
  std::cout << "stopped tracking '" << gname << "'\n";
  return 0;
}

int cmd_groups()
{
  GroupStore store;
  std::string err;
  store.load(err);
  if (store.groups.empty())
  {
    std::cout << "no tracked groups\n";
    std::cout << "start one with: gridot track <name> <pkg>...\n";
    return 0;
  }
  for (const Group& g : store.groups)
  {
    std::cout << g.name << " (" << g.members.size() << ")\n";
    for (const std::string& m : g.members)
      std::cout << "  " << m << "\n";
  }
  return 0;
}

} // namespace

int main(int argc, char** argv)
{
  Options o;
  std::string err;
  if (!parse_args(argc, argv, o, err))
  {
    std::cerr << "gridot: " << err << "\n\n";
    usage();
    return 2;
  }

  if (o.command == "help")
  {
    usage();
    return 0;
  }
  if (o.command == "version")
  {
    std::cout << "gridot " << kVersion << "\n";
    return 0;
  }
  if (o.command == "scan")
    return cmd_scan(o);
  if (o.command == "track")
    return cmd_track(o);
  if (o.command == "untrack")
    return cmd_untrack(o);
  if (o.command == "groups")
    return cmd_groups();

  std::cerr << "gridot: unknown command: " << o.command << "\n\n";
  usage();
  return 2;
}
