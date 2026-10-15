#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

struct Pkg
{
  std::string name;
  std::string arch;
  std::string version;
  std::string section;
  std::string priority;
  bool essential = false;
  bool auto_installed = false;
  bool installed = false;
  long long size_kb = 0;

  std::vector<std::string> depends;
  std::vector<std::string> pre_depends;
  std::vector<std::string> recommends;
  std::vector<std::string> provides;
  std::vector<std::string> file_provides;

  std::string display_size() const;
};

struct Backend
{
  virtual ~Backend() = default;

  virtual std::string name() const = 0;
  virtual bool available() const = 0;
  virtual std::string removal_hint() const = 0;

  virtual bool load(std::string& err) = 0;

  std::map<std::string, Pkg> packages;

  std::map<std::string, std::string> install_origin;

  bool has(const std::string& name) const
  {
    return packages.find(name) != packages.end();
  }
  const Pkg* get(const std::string& name) const;
};

std::unique_ptr<Backend> detect_backend();
