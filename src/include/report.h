#pragma once

#include <set>
#include <string>
#include <vector>

#include "backend.h"
#include "graph.h"
#include "groups.h"

std::string render_report(const Backend& backend,
                          const GraphResult& graph,
                          const GroupStore& store,
                          const std::vector<GroupFinding>& group_findings,
                          const std::set<std::string>& group_roots);
