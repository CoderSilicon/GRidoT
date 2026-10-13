#pragma once

#include <string>
#include <vector>

#include "graph.h"
#include "groups.h"

struct Bucket
{
  std::string label;
  long long size_kb = 0;
  size_t count = 0;
};

struct SectionReport
{
  std::string section;
  long long size_kb = 0;
  size_t count = 0;
};

struct OriginReport
{
  std::string origin;
  long long size_kb = 0;
  size_t count = 0;
};

struct Sections
{
  std::vector<Bucket> buckets;
  std::vector<SectionReport> by_section;
  std::vector<OriginReport> by_origin;
};

Sections build_sections(const std::vector<Orphan>& orphans);
