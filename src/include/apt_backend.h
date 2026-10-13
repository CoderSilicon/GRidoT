#pragma once

#include "backend.h"

class AptBackend final : public Backend
{
public:
  std::string name() const override
  {
    return "apt";
  }
  bool available() const override;
  bool load(std::string& err) override;

private:
  bool load_auto_flags();
  bool load_install_origins();
  static std::vector<std::string> parse_relation(const std::string& raw);
};
