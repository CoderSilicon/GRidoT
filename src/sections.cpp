#include "include/sections.h"

#include <algorithm>
#include <map>

#include "include/util.h"

namespace
{

struct BucketSpec
{
  const char* label;
  long long min_kb;
  long long max_kb; // exclusive
};

const BucketSpec kBuckets[] = {
    {"under 1 MiB", 0, 1024},
    {"1 - 10 MiB", 1024, 10 * 1024},
    {"10 - 50 MiB", 10 * 1024, 50 * 1024},
    {"50 - 200 MiB", 50 * 1024, 200 * 1024},
    {"over 200 MiB", 200 * 1024, 1LL << 62},
};

} // namespace

Sections build_sections(const std::vector<Orphan>& orphans)
{
  Sections s;
  s.buckets.resize(std::size(kBuckets));
  for (size_t i = 0; i < std::size(kBuckets); i++)
    s.buckets[i].label = kBuckets[i].label;

  std::map<std::string, SectionReport> sec;
  std::map<std::string, OriginReport> origin;

  for (const Orphan& o : orphans)
  {
    for (size_t i = 0; i < std::size(kBuckets); i++)
    {
      if (o.size_kb >= kBuckets[i].min_kb && o.size_kb < kBuckets[i].max_kb)
      {
        s.buckets[i].size_kb += o.size_kb;
        s.buckets[i].count++;
        break;
      }
    }
    SectionReport& sr = sec[o.section.empty() ? "unknown" : o.section];
    sr.section = o.section.empty() ? "unknown" : o.section;
    sr.size_kb += o.size_kb;
    sr.count++;

    std::string key = o.pulled_in_by.empty() ? "(no record)" : o.pulled_in_by;
    if (key.size() > 44)
      key = key.substr(0, 42) + "..";
    OriginReport& orr = origin[key];
    orr.origin = key;
    orr.size_kb += o.size_kb;
    orr.count++;
  }

  for (auto& [k, v] : sec)
    s.by_section.push_back(v);
  for (auto& [k, v] : origin)
    s.by_origin.push_back(v);

  std::sort(s.by_section.begin(), s.by_section.end(),
            [](const SectionReport& a, const SectionReport& b)
            {
              if (a.size_kb != b.size_kb)
                return a.size_kb > b.size_kb;
              return a.section < b.section;
            });
  std::sort(s.by_origin.begin(), s.by_origin.end(),
            [](const OriginReport& a, const OriginReport& b)
            {
              if (a.size_kb != b.size_kb)
                return a.size_kb > b.size_kb;
              return a.origin < b.origin;
            });
  return s;
}
