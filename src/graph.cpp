#include "include/graph.h"

#include <algorithm>
#include <deque>
#include <map>

namespace {

bool is_protected(const Pkg& p) {
  if (p.essential) return true;
  return p.priority == "required" || p.priority == "important";
}

std::set<std::string> relations_of(const Pkg& p, bool with_recommends) {
  std::set<std::string> s;
  s.insert(p.pre_depends.begin(), p.pre_depends.end());
  s.insert(p.depends.begin(), p.depends.end());
  // apt installs Recommends by default (APT::Install-Recommends=true), so a
  // recommended package that is installed really is kept alive by the
  // Recommends edge. Ignoring these over-reports by a lot.
  if (with_recommends) s.insert(p.recommends.begin(), p.recommends.end());
  return s;
}

}  // namespace

void Graph::build(const std::map<std::string, Pkg>& packages, bool with_recommends) {
  for (const auto& [name, p] : packages) {
    if (!p.installed) continue;
    for (const std::string& prov : p.provides) {
      if (prov != name) providers[prov].insert(name);
    }
  }

  for (const auto& [name, p] : packages) {
    if (!p.installed) continue;
    std::set<std::string>& out = deps[name];
    for (const std::string& rel : relations_of(p, with_recommends)) {
      // A relation may name a real package or a virtual one.
      if (packages.count(rel) && packages.at(rel).installed) {
        out.insert(rel);
        required_by[rel].insert(name);
        continue;
      }
      auto it = providers.find(rel);
      if (it == providers.end()) continue;
      for (const std::string& real : it->second) {
        out.insert(real);
        required_by[real].insert(name);
      }
    }
  }
}

std::set<std::string> Graph::reachable_from(const std::set<std::string>& roots) const {
  std::set<std::string> seen;
  std::deque<std::string> q;
  for (const std::string& r : roots) {
    if (seen.insert(r).second) q.push_back(r);
  }
  while (!q.empty()) {
    const std::string cur = q.front();
    q.pop_front();
    auto it = deps.find(cur);
    if (it == deps.end()) continue;
    for (const std::string& d : it->second) {
      if (seen.insert(d).second) q.push_back(d);
    }
  }
  return seen;
}

std::vector<std::string> Graph::direct_dependents(const std::string& name) const {
  std::vector<std::string> out;
  auto it = required_by.find(name);
  if (it == required_by.end()) return out;
  out.assign(it->second.begin(), it->second.end());
  return out;
}

GraphResult analyze(const Backend& backend,
                   const std::set<std::string>& extra_roots,
                   bool with_recommends) {
  Graph g;
  g.build(backend.packages, with_recommends);

  std::set<std::string> roots;
  for (const auto& [name, p] : backend.packages) {
    if (!p.installed) continue;
    if (!p.auto_installed || is_protected(p)) roots.insert(name);
  }
  for (const std::string& r : extra_roots) {
    const Pkg* p = backend.get(r);
    if (p && p->installed) roots.insert(r);
  }

  const std::set<std::string> reachable = g.reachable_from(roots);

  // Attribution walks *backwards* along reverse edges. A forward BFS from the
  // roots can never visit an orphan (that is what makes it an orphan), so the
  // "who dragged this in" answer has to be found by climbing from the orphan
  // towards a manual package.
  std::map<std::string, std::pair<std::string, int>> origin;
  {
    const std::set<std::string> root_set(roots.begin(), roots.end());
    for (const auto& [name, p] : backend.packages) {
      if (!p.installed || reachable.count(name)) continue;

      std::set<std::string> seen{name};
      std::deque<std::pair<std::string, int>> q;
      q.emplace_back(name, 0);
      std::string culprit;
      int depth = -1;

      while (!q.empty() && depth < 0) {
        const auto [cur, d] = q.front();
        q.pop_front();
        // Climbing up: cur is needed by these packages.
        auto rit = g.required_by.find(cur);
        if (rit == g.required_by.end()) continue;
        for (const std::string& dependent : rit->second) {
          if (!seen.insert(dependent).second) continue;
          if (root_set.count(dependent)) {
            culprit = dependent;
            depth = d + 1;
            break;
          }
          if (d + 1 <= 16) q.emplace_back(dependent, d + 1);
        }
      }

      if (!culprit.empty()) {
        origin.emplace(name, std::make_pair(culprit, depth));
        continue;
      }
      // No live manual ancestor: fall back to what the package manager itself
      // recorded, which is the only remaining record of why it was installed.
      auto oit = backend.install_origin.find(name);
      if (oit != backend.install_origin.end()) {
        origin.emplace(name, std::make_pair(oit->second, -1));
      }
    }
  }

  GraphResult res;
  for (const auto& [name, p] : backend.packages) {
    if (!p.installed) continue;
    res.installed_count++;
    res.total_installed_kb += p.size_kb;
    if (reachable.count(name)) {
      res.reachable_count++;
      continue;
    }
    Orphan o;
    o.name = name;
    o.section = p.section;
    o.size_kb = p.size_kb;
    o.auto_installed = p.auto_installed;
    auto oit = origin.find(name);
    if (oit != origin.end()) {
      o.pulled_in_by = oit->second.first;
      o.pulled_in_depth = oit->second.second;
    }
    res.orphans.push_back(std::move(o));
    res.total_orphan_kb += p.size_kb;
  }

  std::sort(res.orphans.begin(), res.orphans.end(), [](const Orphan& a, const Orphan& b) {
    if (a.size_kb != b.size_kb) return a.size_kb > b.size_kb;
    return a.name < b.name;
  });
  return res;
}
