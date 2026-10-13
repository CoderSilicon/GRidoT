#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "backend.h"

struct Graph
{
  std::map<std::string, std::set<std::string>> required_by;
  std::map<std::string, std::set<std::string>> providers;
  std::map<std::string, std::set<std::string>> deps;

  void build(const std::map<std::string, Pkg>& packages, bool with_recommends = true);

  std::set<std::string> reachable_from(const std::set<std::string>& roots) const;
  std::vector<std::string> direct_dependents(const std::string& name) const;
};

// An installed package nothing needs any more.
struct Orphan
{
  std::string name;
  std::string section;
  long long size_kb = 0;
  bool auto_installed = false;
  // Which manual package originally dragged this in ("" = unknown/multiple)
  std::string pulled_in_by;
  int pulled_in_depth = 0;
};

struct GraphResult
{
  std::vector<Orphan> orphans;
  long long total_orphan_kb = 0;
  long long total_installed_kb = 0;
  size_t installed_count = 0;
  size_t reachable_count = 0;
};

GraphResult analyze(const Backend& backend, const std::set<std::string>& extra_roots,
                    bool with_recommends = true);
