#pragma once

#include "Command.hpp"

#include <string>
#include <string_view>

class LogicHandler {
 public:
  std::string handle(std::string_view request,
                     CommandContext const& ctx = {}) const;
};
