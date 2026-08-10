#include "commands/bed/bed.h"

namespace crib::commands::bed {
void BEd::parse_address(std::string_view cmd, uint64_t &i, Command::Address &addr) {
  auto skip_space = [&] {
    while (i < cmd.size() && (cmd[i] == ' ' || cmd[i] == '\t'))
      ++i;
  };
  skip_space();
  if (i >= cmd.size()) {
    addr.type = Command::Address::Type::None;
    return;
  }
  switch (cmd[i]) {
  case '.':
    addr.type = Command::Address::Type::Current;
    i++;
    break;
  case '$':
    addr.type = Command::Address::Type::Last;
    i++;
    break;
  case '\'':
    addr.type = Command::Address::Type::Mark;
    i++;
    if (i < cmd.size() && 'a' <= cmd[i] && cmd[i] <= 'z')
      i++;
    else
      throw bed_error("Invalid mark.");
    addr.mark = cmd[i];
    break;
  case '/': {
    addr.type = Command::Address::Type::SearchForward;
    i++;
    uint64_t start = i;
    while (true) {
      if (i >= cmd.size())
        break;
      if (cmd[i] == '/')
        break;
      else if (cmd[i] == '\\')
        i += 2;
      else if (cmd[i] == '[' && i + 1 < cmd.size() && cmd[i + 1] == '[')
        while (i < cmd.size() && !(cmd[i - 1] == ']' && cmd[i] == ']'))
          i++;
      else if (cmd[i] == '[')
        while (i < cmd.size() && cmd[i] != ']')
          i++;
      else
        i++;
    }
    addr.regex = cmd.substr(start, i - start);
  } break;
  case '?': {
    addr.type = Command::Address::Type::SearchBackward;
    i++;
    uint64_t start = i;
    while (true) {
      if (i >= cmd.size())
        break;
      if (cmd[i] == '?')
        break;
      else if (cmd[i] == '\\')
        i += 2;
      else if (cmd[i] == '[' && i + 1 < cmd.size() && cmd[i + 1] == '[')
        while (i < cmd.size() && !(cmd[i - 1] == ']' && cmd[i] == ']'))
          i++;
      else if (cmd[i] == '[')
        while (i < cmd.size() && cmd[i] != ']')
          i++;
      else
        i++;
    }
    addr.regex = cmd.substr(start, i - start);
  } break;
  case '+': {
    i++;
    skip_space();
    uint64_t start = i;
    int64_t num = 0;
    while (i < cmd.size() && '0' <= cmd[i] && cmd[i] <= '9') {
      num = num * 10 + (cmd[i] - '0');
      i++;
    }
    if (start == i)
      num = 1;
    addr.offset += num;
  } break;
  case '-': {
    i++;
    skip_space();
    uint64_t start = i;
    int64_t num = 0;
    while (i < cmd.size() && '0' <= cmd[i] && cmd[i] <= '9') {
      num = num * 10 + (cmd[i] - '0');
      i++;
    }
    if (start == i)
      num = 1;
    addr.offset -= num;
  } break;
  default: {
    if ('0' <= cmd[i] && cmd[i] <= '9') {
      int64_t num = 0;
      addr.type = Command::Address::Type::Number;
      while (i < cmd.size() && '0' <= cmd[i] && cmd[i] <= '9') {
        num = num * 10 + (cmd[i] - '0');
        i++;
      }
      addr.number = num;
    } else {
      addr.type = Command::Address::Type::None;
      return;
    }
    break;
  }
  }
  skip_space();
  while (i < cmd.size() && (cmd[i] == '+' || cmd[i] == '-' || ('0' <= cmd[i] && cmd[i] <= '9'))) {
    bool positive = cmd[i] != '-';
    if (cmd[i] == '+' || cmd[i] == '-') {
      i++;
      skip_space();
    }
    uint64_t start = i;
    int64_t num = 0;
    while (i < cmd.size() && '0' <= cmd[i] && cmd[i] <= '9') {
      num = num * 10 + (cmd[i] - '0');
      i++;
    }
    if (start == i)
      num = 1;
    addr.offset += positive ? num : -num;
    skip_space();
  }
}

void BEd::resolve_address(Command::Address addr, uint64_t *out_line) {
  switch (addr.type) {
  case Command::Address::Type::None:
    break;
  case Command::Address::Type::Current:
    *out_line = line;
    if (*out_line > vase.lines())
      *out_line = vase.lines();
    break;
  case Command::Address::Type::Number:
    *out_line = addr.number;
    break;
  case Command::Address::Type::Last:
    *out_line = vase.lines();
    break;
  case Command::Address::Type::Mark:
    *out_line = marks[addr.mark - 'a'];
    if (!*out_line)
      throw bed_error("Unset mark used.");
    break;
  case Command::Address::Type::SearchForward:
    if (addr.regex == "")
      addr.regex = last_regex;
    last_regex = addr.regex;
    // TODO: use vase.regex_search(regex, range, options);
    break;
  case Command::Address::Type::SearchBackward:
    if (addr.regex == "")
      addr.regex = last_regex;
    last_regex = addr.regex;
    // TODO
    break;
  }
  if ((int64_t)*out_line + addr.offset < 0)
    throw bed_error("Line position can't be negative.");
  else
    *out_line += addr.offset;
  if (*out_line > vase.lines())
    throw bed_error("Line position too high.");
}
} // namespace crib::commands::bed
