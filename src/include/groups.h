#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "backend.h"

// A user-declared use case, e.g. `gridot track sway swaybg waybar`.
// Members are treated as extra roots so a tracked bundle is never reported
// as waste. When one member disappears, the rest become "at risk".
struct Group
{
  std::string name;
  std::vector<std::string> members;
};

struct GroupStore
{
  std::vector<Group> groups;

  static std::string path();
  bool load(std::string& err);
  bool save(std::string& err) const;

  Group* find(const std::string& name);
  const Group* find(const std::string& name) const;
  void add(const std::string& name, const std::vector<std::string>& members);
  bool remove(const std::string& name);
  std::vector<std::string> groups_containing(const std::string& pkg) const;
  std::set<std::string> all_members() const;
  std::set<std::string> names() const;
};

struct GroupFinding
{
  std::string group;
  std::string member;
  std::string reason;
  bool member_installed = false;
  long long size_kb = 0;
  // Still needed by something outside the group?
  bool externally_needed = false;
  std::vector<std::string> external_dependents;
  // Reachable only because a sibling in the same group kept it alive.
  bool at_risk = false;
};

std::vector<GroupFinding> audit_groups(const GroupStore& store, const Backend& backend);
