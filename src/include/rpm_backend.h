#pragma once

#include <map>
#include <set>
#include <string>

#include "backend.h"

class RpmBackend final : public Backend
{
public:
  std::string name() const override;
  bool available() const override;
  bool load(std::string& err) override;
  std::string removal_hint() const override;

private:
  bool load_packages(std::string& err);
  bool load_install_reasons(std::string& err);
  bool load_yumdb_reasons();
  void load_protections();

  std::map<std::string, std::string> nvra_to_name;
  std::set<std::string> multiversion;
};
