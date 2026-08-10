#include "commands/bed/bed.h"

namespace crib::commands::bed {
Command BEd::parse(std::string cmd, bool eof) {
  Command command = {};
  if (eof)
    return (command.type = Command::Type::Quit, command);
  uint64_t i = 0;
  auto skip_space = [&] {
    while (i < cmd.size() && (cmd[i] == ' ' || cmd[i] == '\t'))
      ++i;
  };
  Command::Address first;
  Command::Address second;
  parse_address(cmd, i, first);
  bool have_range = false;
  while (i < cmd.size() && (cmd[i] == ',' || cmd[i] == ';')) {
    char sep = cmd[i++];
    if (sep == ';')
      resolve_address(first, &line);
    command.address_flags = sep == ';' ? Command::SEMICOLON : 0;
    if (have_range)
      first = second;
    parse_address(cmd, i, second);
    have_range = true;
  }
  if (have_range) {
    if (first.type != Command::Address::Type::None
        && second.type == Command::Address::Type::None) {
      second = first;
    } else if (first.type == Command::Address::Type::None) {
      if (command.address_flags & Command::SEMICOLON) {
        first.type = Command::Address::Type::Current;
      } else {
        first.type = Command::Address::Type::Number;
        first.number = 1;
      }
      if (second.type == Command::Address::Type::None)
        second.type = Command::Address::Type::Last;
    }
    command.address_flags |= Command::RANGE;
    command.start = first;
    command.end = second;
  } else {
    command.start = first;
  }
  if (i >= cmd.size()) {
    command.type = Command::Type::None;
    return command;
  }
  switch (cmd[i]) {
  case 'q':
    command.type = Command::Type::Quit;
    return command;
  case 'Q':
    command.type = Command::Type::ForceQuit;
    return command;
  case 'p':
    command.type = Command::Type::Print;
    i++;
    break;
  case 'l':
    command.type = Command::Type::List;
    i++;
    break;
  case 'n':
    command.type = Command::Type::Number;
    i++;
    break;
  case 'P':
    command.type = Command::Type::PromptToggle;
    i++;
    break;
  case 'H':
    command.type = Command::Type::HelpToggle;
    i++;
    break;
  case 'h':
    command.type = Command::Type::Help;
    i++;
    break;
  case 'a':
    command.type = Command::Type::Append;
    i++;
    break;
  case 'i':
    command.type = Command::Type::Insert;
    i++;
    break;
  case 'c':
    command.type = Command::Type::Change;
    i++;
    break;
  case 'd':
    command.type = Command::Type::Delete;
    i++;
    break;
  case 'j':
    command.type = Command::Type::Join;
    i++;
    break;
  case 'w':
    command.type = Command::Type::Write;
    i++;
    if (i >= cmd.size())
      return command;
    if (cmd[i] == ' ' || cmd[i] == '\t')
      skip_space();
    else
      throw bed_error("Invalid command.");
    command.argument = cmd.substr(i);
    return command;
  case 'e':
    command.type = Command::Type::Edit;
    i++;
    if (i >= cmd.size())
      return command;
    if (cmd[i] == ' ' || cmd[i] == '\t')
      skip_space();
    else
      throw bed_error("Invalid command.");
    command.argument = cmd.substr(i);
    return command;
  case 'E':
    command.type = Command::Type::ForceEdit;
    i++;
    if (i >= cmd.size())
      return command;
    if (cmd[i] == ' ' || cmd[i] == '\t')
      skip_space();
    else
      throw bed_error("Invalid command.");
    command.argument = cmd.substr(i);
    return command;
  case 'f':
    command.type = Command::Type::Filename;
    i++;
    if (i >= cmd.size())
      return command;
    if (cmd[i] == ' ' || cmd[i] == '\t')
      skip_space();
    else
      throw bed_error("Invalid command.");
    command.argument = cmd.substr(i);
    return command;
  case 'k':
    command.type = Command::Type::Mark;
    i++;
    if (i >= cmd.size())
      throw bed_error("Invalid command.");
    if ('a' <= cmd[i] && cmd[i] <= 'z')
      command.argument = cmd[i];
    else
      throw bed_error("Invalid mark used.");
    break;
  case '#':
    command.type = Command::Type::Dump;
    i++;
    break;
  }
  if (i < cmd.size()) {
    if (cmd[i] == 'l' || cmd[i] == 'n' || cmd[i] == 'p')
      command.suffix = cmd[i++];
    skip_space();
    if (i != cmd.size())
      throw bed_error("Invalid command.");
  }
  return command;
}
} // namespace crib::commands::bed
