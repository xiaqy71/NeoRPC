#include "Command.hpp"
#include "LogicHandler.hpp"

#include <string>

#define COMMAND_INCLUDE_SECTION
#include "CommandList.def"
#undef COMMAND_INCLUDE_SECTION

namespace {

template <typename CommandType>
std::string dispatchCommand(std::string_view args, CommandContext const& ctx) {
  CommandType command;
  return command.execute(args, ctx);
}

}  // namespace

std::string LogicHandler::handle(std::string_view request,
                                 CommandContext const& ctx) const {
  const auto first_space = request.find(' ');
  const std::string_view command = first_space == std::string_view::npos
                                       ? request
                                       : request.substr(0, first_space);
  const std::string_view args = first_space == std::string_view::npos
                                    ? std::string_view{}
                                    : request.substr(first_space + 1);

#define COMMAND_ENTRY(name, type) \
  if (command == #name) {  \
    return dispatchCommand<type>(args, ctx); \
  }
#include "CommandList.def"
#undef COMMAND_ENTRY

  return "ERR unknown command";
}
