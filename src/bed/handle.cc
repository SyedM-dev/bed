#include "bed.h"
#include "internal/address/address.h"
#include "internal/commands/commands.h"

namespace bed {
void BEd::handle(std::string_view cmd_, bool eof) {
  using namespace internal::commands;
  std::string cmd(cmd_);
  if (eof) {
    eof_op.handle(*this, {}, "");
    return;
  }
  uint64_t i = 0;
  auto skip_space = [&] {
    while (i < cmd.size() && (cmd[i] == ' ' || cmd[i] == '\t'))
      ++i;
  };
  skip_space();
  auto addresses = internal::address::Address::handle(*this, cmd, i);
  if (i >= cmd.size()) {
    if (!no_op.accept_zero)
      for (auto l : addresses.span())
        if (l == 0)
          throw ed_error("Invalid address given.");
    switch (no_op.address_mode) {
    case Command::AddressMode::None:
      no_op.handle(*this, addresses.span(0), "");
      break;
    case Command::AddressMode::Single:
      no_op.handle(*this, addresses.span(1), "");
      break;
    case Command::AddressMode::Range:
      no_op.handle(*this, addresses.span(2), "");
      break;
    }
    return;
  }
  uint64_t len = commands.longest_match(cmd.substr(i));
  if (len == 0)
    throw ed_error("Command not found.");
  std::optional<Command> command_opt = commands.get(cmd.substr(i, len));
  i += len;
  if (!command_opt)
    throw ed_error("Command error.");
  Command command = command_opt.value();
  std::string argument;
  Suffix *suffix = nullptr;
  switch (command.suffix) {
  case Command::SuffixKind::None:
    skip_space();
    if (i < cmd.size())
      throw ed_error("Command error.");
    break;
  case Command::SuffixKind::Suffix:
    skip_space();
    if (i < cmd.size()) {
      if (suffixes[cmd[i] - 'a'].has_value())
        suffix = &(suffixes[cmd[i++] - 'a'].value());
      skip_space();
      std::cout << i << " " << cmd << std::endl;
      if (i < cmd.size())
        throw ed_error("Command error.");
    }
    break;
  case Command::SuffixKind::Argument:
    if (i < cmd.size()) {
      if (cmd[i] == ' ' || cmd[i] == '\t')
        skip_space();
      else
        throw ed_error("Command Error.");
      argument = cmd.substr(i);
    }
    break;
  case Command::SuffixKind::Continuation:
    argument = cmd.substr(i);
    break;
  }
  if (!command.accept_zero)
    for (auto l : addresses.span())
      if (l == 0)
        throw ed_error("Invalid address given.");
  switch (command.address_mode) {
  case Command::AddressMode::None:
    command.handle(*this, addresses.span(0), argument);
    break;
  case Command::AddressMode::Single:
    command.handle(*this, addresses.span(1), argument);
    break;
  case Command::AddressMode::Range:
    command.handle(*this, addresses.span(2), argument);
    break;
  }
  if (suffix)
    suffix->handle(*this);
  return;
}
} // namespace bed
