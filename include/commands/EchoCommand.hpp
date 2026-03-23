#pragma once

#include "Command.hpp"

class EchoCommand : public Command {
 public:
  std::string execute(std::string_view args,
                      CommandContext const& ctx) const override {
    (void)ctx;
    return std::string(args);
  }
};
