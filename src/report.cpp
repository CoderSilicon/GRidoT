#include "include/report.h"

#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "include/sections.h"
#include "include/util.h"

namespace
{

const char* kReset = "\033[0m";
const char* kBold = "\033[1m";
const char* kDim = "\033[2m";
const char* kRed = "\033[31m";
const char* kYellow = "\033[33m";
const char* kGreen = "\033[32m";
const char* kCyan = "\033[36m";

bool use_color()
{
  static const bool t = []
  {
    if (std::getenv("NO_COLOR"))
      return false;
    const char* tty = std::getenv("TERM");
    if (tty && std::string(tty) == "dumb")
      return false;
    return ::isatty(1) != 0;
  }();
  return t;
}

std::string c(const char* code, const std::string& s)
{
  if (!use_color())
    return s;
  return std::string(code) + s + kReset;
}

std::string pad_right(const std::string& s, size_t w)
{
  if (s.size() >= w)
    return s;
  return s + std::string(w - s.size(), ' ');
}

std::string pad_left(const std::string& s, size_t w)
{
  if (s.size() >= w)
    return s;
  return std::string(w - s.size(), ' ') + s;
}

std::string rule(char ch = '-', int width = 78)
{
  return std::string(width, ch);
}

std::string plural(size_t n, const char* one, const char* many)
{
  return std::to_string(n) + " " + (n == 1 ? one : many);
}

std::string distro_line()
{
  std::string data;
  if (!util::read_file("/etc/os-release", data))
    return "unknown";
  for (const std::string& line : util::split(data, '\n'))
  {
    if (util::starts_with(line, "PRETTY_NAME="))
    {
      std::string v = util::trim(line.substr(12));
      if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
        v = v.substr(1, v.size() - 2);
      return v;
    }
  }
  return "unknown";
}

void print_kv(std::ostream& o, const std::string& k, const std::string& v, int indent = 2)
{
  o << std::string(indent, ' ') << c(kDim, pad_right(k, 22)) << v << '\n';
}

} // namespace

std::string render_report(const Backend& backend, const GraphResult& graph, const GroupStore& store,
                          const std::vector<GroupFinding>& group_findings,
                          const std::set<std::string>& group_roots)
{
  std::ostringstream o;
  const Sections sec = build_sections(graph.orphans);

  o << '\n' << c(kBold, "GRidoT") << "  " << c(kDim, "package waste report") << '\n';
  o << c(kDim, distro_line()) << "  " << c(kCyan, backend.name()) << " backend\n";
  o << rule() << '\n';

  print_kv(o, "installed packages", std::to_string(graph.installed_count));
  print_kv(o, "installed size", util::human_size(graph.total_installed_kb));
  print_kv(o, "orphaned", std::to_string(graph.orphans.size()));
  print_kv(o, "reclaimable",
           c(graph.orphans.empty() ? kGreen : kYellow, util::human_size(graph.total_orphan_kb)));

  // ---- Group-aware additions -------------------------------------------------
  size_t tracked = 0;
  long long tracked_kb = 0;
  for (const std::string& r : group_roots)
  {
    if (const Pkg* p = backend.get(r); p && p->installed)
    {
      tracked++;
      tracked_kb += p->size_kb;
    }
  }
  if (tracked)
  {
    print_kv(o, "tracked as roots",
             plural(tracked, "package", "packages") + " (" + util::human_size(tracked_kb) + ")");
  }
  o << '\n';

  if (graph.orphans.empty())
  {
    o << "  " << c(kGreen, "No orphaned packages found.") << '\n';
    o << "  " << c(kDim, "Nothing is sitting unused in the dependency graph.") << '\n';
    o << '\n' << rule('=') << '\n';
    return o.str();
  }

  // ---- Bucket overview -------------------------------------------------------
  o << c(kBold, "  BY SIZE") << '\n';
  for (const Bucket& b : sec.buckets)
  {
    if (b.count == 0)
      continue;
    const int total_kb =
        graph.total_orphan_kb > 0 ? static_cast<int>((b.size_kb * 100) / graph.total_orphan_kb) : 0;
    o << "  " << pad_right(b.label, 14) << pad_left(util::human_size(b.size_kb), 10) << "  "
      << pad_left(plural(b.count, "pkg", "pkgs"), 9) << "  " << c(kDim, std::string(20, '.')) << " "
      << pad_left(std::to_string(total_kb) + "%", 5) << '\n';
  }
  o << '\n';

  // ---- Why they are here -----------------------------------------------------
  if (!sec.by_origin.empty())
  {
    o << c(kBold, "  HISTORY");
    if (!backend.install_origin.empty())
      o << " " << c(kDim, "(from " + backend.name() + ")");
    o << '\n';
    size_t shown = 0;
    for (const OriginReport& r : sec.by_origin)
    {
      if (shown++ >= 12)
        break;
      o << "  " << pad_right(r.origin, 46) << pad_left(util::human_size(r.size_kb), 10) << "  "
        << c(kDim, plural(r.count, "pkg", "pkgs")) << '\n';
    }
    o << '\n';
  }

  // ---- What kind of thing --------------------------------------------------
  if (!sec.by_section.empty())
  {
    o << c(kBold, "  INTERNAL GROUP") << '\n';
    size_t shown = 0;
    for (const SectionReport& r : sec.by_section)
    {
      if (shown++ >= 12)
        break;
      o << "  " << pad_right(r.section, 32) << pad_left(util::human_size(r.size_kb), 10) << "  "
        << c(kDim, plural(r.count, "pkg", "pkgs")) << '\n';
    }
    o << '\n';
  }

  // ---- Full list ------------------------------------------------------------
  o << c(kBold, "  FLAGGED PACKAGES") << " "
    << c(kDim, "(" + std::to_string(graph.orphans.size()) + " total, largest first)") << '\n';
  o << "  "
    << c(kDim, pad_right("SIZE", 10) + " " + pad_right("PACKAGE", 34) + " " +
                   pad_right("GROUP", 18) + " FROM")
    << '\n';

  std::set<std::string> group_members = store.all_members();
  for (const Orphan& op : graph.orphans)
  {
    const bool tracked = group_members.count(op.name) > 0;
    std::string origin = op.pulled_in_by.empty() ? "(null)" : op.pulled_in_by;
    if (op.pulled_in_depth > 0)
    {
      origin += " +" + std::to_string(op.pulled_in_depth);
    }
    if (origin.size() > 46)
      origin = origin.substr(0, 45) + "\xe2\x80\xa6";
    o << "  " << pad_left(util::human_size(op.size_kb), 10) << " " << pad_right(op.name, 34) << " "
      << pad_right(op.section, 18) << " " << c(kDim, origin);
    if (tracked)
      o << " " << c(kCyan, "[tracked]");
    if (!op.auto_installed)
      o << " " << c(kYellow, "[manual]");
    o << '\n';
  }
  o << '\n';

  // ---- Group audit ----------------------------------------------------------
  if (!group_findings.empty())
  {
    o << rule() << '\n';
    o << c(kBold, "  TRACKED GROUPS") << '\n';
    if (store.groups.empty())
    {
      o << "  " << c(kDim, "no groups tracked yet") << '\n';
      o << "  " << c(kDim, "group a use case with:  gridot track <name> <pkg>...") << '\n';
    }

    std::map<std::string, std::vector<const GroupFinding*>> by_group;
    for (const GroupFinding& f : group_findings)
      by_group[f.group].push_back(&f);

    for (const auto& [gname, fs] : by_group)
    {
      bool any_risk = false, any_missing = false;
      for (const GroupFinding* f : fs)
      {
        any_risk |= f->at_risk;
        any_missing |= !f->member_installed;
      }
      const char* tag = any_missing ? kRed : (any_risk ? kYellow : kGreen);
      const char* word = any_missing ? "BROKEN" : (any_risk ? "AT RISK" : "OK");
      o << "\n  " << c(tag, pad_right(std::string("[") + word + "]", 12)) << c(kBold, gname)
        << '\n';

      for (const GroupFinding* f : fs)
      {
        if (!f->member_installed)
        {
          o << "      " << c(kRed, "- ") << pad_right(f->member, 30) << c(kDim, f->reason) << '\n';
        }
        else if (f->at_risk)
        {
          o << "      " << c(kYellow, "! ") << pad_right(f->member, 30)
            << pad_left(util::human_size(f->size_kb), 10) << "  " << c(kDim, f->reason) << '\n';
        }
        else
        {
          o << "      " << c(kGreen, "+ ") << pad_right(f->member, 30)
            << pad_left(util::human_size(f->size_kb), 10) << "  " << c(kDim, f->reason) << '\n';
        }
      }
    }
    o << '\n';
  }

  o << rule('=') << '\n';
  o << "  "
    << c(kBold, std::to_string(graph.orphans.size()) + " orphaned, " +
                    util::human_size(graph.total_orphan_kb) + " reclaimable")
    << '\n';
  o << "  "
    << c(kDim, "This is only a report and maybe wrong, please verify them before proceeding.")
    << '\n';
  o << "  " << c(kDim, "To remove them, review the list first then run:") << " "
    << c(kCyan, backend.removal_hint()) << '\n';
  o << '\n';
  return o.str();
}
