#pragma once

#include <string>
#include <vector>

namespace util
{

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
std::string join(const std::vector<std::string>& parts, const std::string& sep);
bool starts_with(const std::string& s, const std::string& prefix);
bool contains(const std::string& s, const std::string& needle);
std::string to_lower(const std::string& s);

bool read_file(const std::string& path, std::string& out);
bool write_file(const std::string& path, const std::string& data);
bool file_exists(const std::string& path);
bool make_dirs(const std::string& path);

struct ProcResult
{
  int exit_code = -1;
  std::string out;
  bool spawned = false;
};

ProcResult run(const std::string& cmd);
bool have_cmd(const std::string& cmd);

std::string human_size(long long kb);
std::string config_dir();

} // namespace util
