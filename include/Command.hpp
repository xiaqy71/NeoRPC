#pragma once

#include "ServiceRegistry.hpp"

#include <string>
#include <string_view>

struct CommandContext {
  int connection_fd{-1};
  ServiceRegistry* services{nullptr};
};

class Command {
 public:
  virtual ~Command() = default;
  virtual std::string execute(std::string_view args,
                              CommandContext const& ctx) const = 0;
};
