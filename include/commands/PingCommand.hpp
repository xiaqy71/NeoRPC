#pragma once

#include "Command.hpp"

class PingCommand : public Command {
 public:
  std::string execute(std::string_view args,
                      CommandContext const& ctx) const override {
    (void)ctx;
    if (!args.empty()) {
      return "ERR ping takes no arguments";
    }
    return "pong";
  }
};
