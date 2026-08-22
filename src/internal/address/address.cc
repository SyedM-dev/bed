#include "internal/address/address.h"

namespace bed::internal::address {
Address::Address(std::string &cmd, uint64_t &i) {
  base = None{};
  auto skip_space = [&] {
    while (i < cmd.size() && (cmd[i] == ' ' || cmd[i] == '\t'))
      ++i;
  };
  skip_space();
  if (i >= cmd.size())
    return;
  switch (cmd[i]) {
  case '.':
    base = Current();
    i++;
    break;
  case '$':
    base = Last();
    i++;
    break;
  case ']':
    base = Block(Direction::Forward);
    i++;
    break;
  case '[':
    base = Block(Direction::Backward);
    i++;
    break;
  case '\'': {
    i++;
    if (i < cmd.size()
        && (('a' <= cmd[i] && cmd[i] <= 'z') || ('A' <= cmd[i] && cmd[i] <= 'Z')))
      i++;
    else
      throw address_error("Invalid mark.");
    base = Mark(cmd[i - 1]);
  } break;
  case '/': {
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
    base = Regex(Direction::Forward, cmd.substr(start, i - start));
    if (i < cmd.size())
      i++;
  } break;
  case '?': {
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
    base = Regex(Direction::Backward, cmd.substr(start, i - start));
    if (i < cmd.size())
      i++;
  } break;
  case '+': {
    base = Current();
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
    offset += num;
  } break;
  case '-': {
    base = Current();
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
    offset -= num;
  } break;
  default: {
    if ('0' <= cmd[i] && cmd[i] <= '9') {
      uint64_t num = 0;
      while (i < cmd.size() && '0' <= cmd[i] && cmd[i] <= '9') {
        num = num * 10 + (cmd[i] - '0');
        i++;
      }
      base = Number(num);
    } else {
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
    offset += positive ? num : -num;
    skip_space();
  }
}
} // namespace bed::internal::address
