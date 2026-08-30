#include "bed.h"
#include "internal/parser/parser.h"

namespace bed::internal::parser {
void Parser::operation() {
  if (peek() == '\0') {
    command->function = &bed.no_op;
    return;
  }
  uint64_t len = bed.functions.longest_match(peek_str());
  if (len == 0)
    throw ed_error("Function not found.");
  functions::Function *function = bed.functions.get_ptr(peek_str(len));
  advance(len);
  command->function = function;
  char suffix = '\0';
  switch (command->function->argument_kind) {
  case functions::Function::ArgumentKind::None:
    break;
  case functions::Function::ArgumentKind::Number:
    skip_ws();
    command->argument = offset();
    break;
  case functions::Function::ArgumentKind::Mark:
    if (('a' <= peek() && peek() <= 'z')
        || ('A' <= peek() && peek() <= 'Z'))
      command->argument = peek();
    else
      throw ed_error("Valid mark needed.");
    advance();
    break;
  case functions::Function::ArgumentKind::Any:
    command->argument = std::string(peek_str());
    advance(peek_str().size());
    break;
  case functions::Function::ArgumentKind::Global: {
    char delim;
    std::string val;
    switch (peek()) {
    case '\0':
      throw ed_error("Command needs a delimited value.");
    case '{': {
      advance();
      delim = '}';
      uint16_t j = 0;
      while (peek(j) != '}' && peek(j) != '\0') {
        if (peek(j) == '\\')
          j++;
        j++;
      }
      switch (peek(j)) {
      case '\0':
        throw ed_error("Unterminated {");
      case '}':
        val = peek_str(j);
        break;
      }
      advance(j + 1);
    } break;
    case '<': {
      advance();
      delim = '<';
      uint16_t j = 0;
      while (peek(j) != '>' || peek(j) != '\0') {
        if (peek(j) == '\\')
          j++;
        j++;
      }
      switch (peek(j)) {
      case '\0':
        throw ed_error("Unterminated <");
      case '>':
        val = peek_str(j);
        break;
      }
      advance(j + 1);
    } break;
    case '^':
    case '~':
      advance();
      delim = '^';
      break;
    default:
      delim = peek();
      advance();
      uint64_t j = 0;
      while (true) {
        if (peek(j) == '\0')
          break;
        if (peek(j) == delim)
          break;
        else if (peek(j) == '\\')
          j += 2;
        else if (peek(j) == '[')
          while (peek(j) != '\0' && cmd[i] != ']')
            j++;
        else
          j++;
      }
      val = peek_str(j);
      advance(j + 1);
    }
    command->argument = functions::Function::GlobalArg(delim, std::move(val));
  } break;
  case functions::Function::ArgumentKind::File:
    skip_ws();
    switch (peek()) {
    case '!':
      advance();
      command->argument = functions::Function::ShellArg(std::string(peek_str()));
      advance(peek_str().size());
      break;
    case '\0':
      command->argument = std::monostate();
      break;
    default:
      command->argument = std::filesystem::path(peek_str());
      advance(peek_str().size());
      break;
    }
    break;
  case functions::Function::ArgumentKind::Line:
    command->argument = buffer::Line();
    addresses(command->argument_addresses);
    break;
  case functions::Function::ArgumentKind::Range:
    command->argument = buffer::Range();
    addresses(command->argument_addresses);
    break;
  case functions::Function::ArgumentKind::Regex: {
    char delim = peek();
    if (delim == '\0')
      throw ed_error("regex expected");
    advance();
    uint64_t j = 0;
    while (true) {
      if (peek(j) == '\0')
        throw ed_error("Unterminated regex");
      if (peek(j) == delim)
        break;
      else if (peek(j) == '\\')
        j += 2;
      else if (peek(j) == '[')
        while (peek(j) != '\0' && cmd[i] != ']')
          j++;
      else
        j++;
    }
    std::string expression(peek_str(j));
    advance(j + 1);
    j = 0;
    while (true) {
      if (peek(j) == '\0')
        break;
      if (peek(j) == delim)
        break;
      else if (peek(j) == '\\')
        j += 2;
      else if (peek(j) == '[')
        while (peek(j) != '\0' && cmd[i] != ']')
          j++;
      else
        j++;
    }
    std::string replacement;
    if (peek(j) != '\0') {
      replacement = peek_str(j);
      advance(j + 1);
    }
    std::string options;
    if (peek() != '\0') {
      options = std::string(peek_str());
      advance(peek_str().size());
    }
    std::erase_if(options, [&](char c) {
      if (bed.suffixes[c - 'a'].has_value()) {
        suffix = c;
        return true;
      }
      return false;
    });
    command->argument = functions::Function::RegexArg(expression, replacement, options);
  } break;
  case functions::Function::ArgumentKind::Ruby:
    command->argument = functions::Function::RubyArg(std::string(peek_str()));
    advance(peek_str().size());
    break;
  case functions::Function::ArgumentKind::Shell:
    command->argument = functions::Function::ShellArg(std::string(peek_str()));
    advance(peek_str().size());
    break;
  }
  if (!suffix) {
    suffix = peek();
    advance();
  }
  if (suffix) {
    auto &s = bed.suffixes[suffix - 'a'];
    if (s.has_value())
      command->suffix = &s.value();
    else
      throw ed_error("Invalid suffix.");
  }
}
} // namespace bed::internal::parser
