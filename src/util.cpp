#include "include/util.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace util {

std::string trim(const std::string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) b++;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
  return s.substr(b, e - b);
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == delim) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
  std::string out;
  for (size_t i = 0; i < parts.size(); i++) {
    if (i) out += sep;
    out += parts[i];
  }
  return out;
}

bool starts_with(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool contains(const std::string& s, const std::string& needle) {
  return s.find(needle) != std::string::npos;
}

std::string to_lower(const std::string& s) {
  std::string out = s;
  for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

bool read_file(const std::string& path, std::string& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

bool write_file(const std::string& path, const std::string& data) {
  std::ofstream o(path, std::ios::binary | std::ios::trunc);
  if (!o) return false;
  o << data;
  return o.good();
}

bool file_exists(const std::string& path) {
  struct stat st;
  return ::stat(path.c_str(), &st) == 0;
}

bool make_dirs(const std::string& path) {
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return !ec;
}

bool have_cmd(const std::string& cmd) {
  if (cmd.empty()) return false;
  const std::string probe = "command -v " + cmd + " >/dev/null 2>&1";
  return std::system(probe.c_str()) == 0;
}

ProcResult run(const std::string& cmd) {
  ProcResult r;
  r.spawned = true;
  FILE* p = ::popen((cmd + " 2>/dev/null").c_str(), "r");
  if (!p) {
    r.spawned = false;
    return r;
  }
  std::array<char, 4096> buf{};
  size_t n = 0;
  while ((n = std::fread(buf.data(), 1, buf.size(), p)) > 0) r.out.append(buf.data(), n);
  int status = ::pclose(p);
  if (status != -1 && WIFEXITED(status)) {
    r.exit_code = WEXITSTATUS(status);
  } else {
    r.exit_code = -1;
  }
  return r;
}

std::string human_size(long long kb) {
  if (kb < 0) kb = 0;
  static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
  double v = static_cast<double>(kb) * 1024.0;
  int u = 0;
  while (v >= 1024.0 && u < 4) {
    v /= 1024.0;
    u++;
  }
  char buf[64];
  if (v < 10.0 && u > 0) {
    std::snprintf(buf, sizeof(buf), "%.1f %s", v, units[u]);
  } else {
    std::snprintf(buf, sizeof(buf), "%.0f %s", v, units[u]);
  }
  return buf;
}

static std::string xdg_dir(const char* env, const char* fallback) {
  if (const char* v = std::getenv(env); v && *v) return v;
  if (const char* h = std::getenv("HOME"); h && *h)
    return std::string(h) + "/" + fallback;
  return std::string();
}

std::string config_dir() { return xdg_dir("XDG_CONFIG_HOME", ".config") + "/gridot"; }

}  // namespace util
