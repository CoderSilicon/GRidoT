#include "../include/backend.h"

#include "../include/apt_backend.h"
#include "../include/util.h"

std::string Pkg::display_size() const { return util::human_size(size_kb); }

const Pkg* Backend::get(const std::string& name) const {
  auto it = packages.find(name);
  return it == packages.end() ? nullptr : &it->second;
}

std::unique_ptr<Backend> detect_backend() {
  if (util::file_exists("/usr/bin/dpkg-query") || util::file_exists("/bin/dpkg-query")) {
    return std::make_unique<AptBackend>();
  }
  return nullptr;
}
