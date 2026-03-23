#pragma once

#include "Command.hpp"
#include "services/StatsService.hpp"

class StatsCommand : public Command {
 public:
  std::string execute(std::string_view args,
                      CommandContext const& ctx) const override {
    if (!args.empty()) {
      return "ERR stats takes no arguments";
    }
    if (ctx.services == nullptr || !ctx.services->contains<StatsService>()) {
      return "ERR stats service unavailable";
    }
    return ctx.services->get<StatsService>()->snapshot();
  }
};
