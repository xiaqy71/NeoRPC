#pragma once

#include "Command.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class TimeCommand : public Command {
 public:
  std::string execute(std::string_view args,
                      CommandContext const& ctx) const override {
    (void)ctx;
    if (!args.empty()) {
      return "ERR time takes no arguments";
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }
};
