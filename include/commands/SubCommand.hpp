#pragma once

#include "Command.hpp"

#include <charconv>
#include <string>

class SubCommand : public Command {
 private:
  static bool parseTwoInts(std::string_view args, int& lhs, int& rhs) {
    const auto sep = args.find(' ');
    if (sep == std::string_view::npos) {
      return false;
    }
    const std::string_view left = args.substr(0, sep);
    const std::string_view right = args.substr(sep + 1);
    if (left.empty() || right.empty()) {
      return false;
    }
    auto [p1, ec1] = std::from_chars(left.data(), left.data() + left.size(), lhs);
    auto [p2, ec2] =
        std::from_chars(right.data(), right.data() + right.size(), rhs);
    return ec1 == std::errc{} && ec2 == std::errc{} &&
           p1 == left.data() + left.size() &&
           p2 == right.data() + right.size();
  }

 public:
  std::string execute(std::string_view args,
                      CommandContext const& ctx) const override {
    (void)ctx;
    int lhs = 0;
    int rhs = 0;
    if (!parseTwoInts(args, lhs, rhs)) {
      return "ERR usage: sub <int> <int>";
    }
    return std::to_string(lhs - rhs);
  }
};
