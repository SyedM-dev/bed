#include "internal/parser/parser.h"
#include "bed.h"

namespace bed::internal::parser {
char Parser::peek(uint16_t o) {
  return i + o < cmd.size() ? cmd[i + o] : '\0';
}

std::string_view Parser::peek_str(uint16_t len) {
  return cmd.substr(i, len);
}

void Parser::advance(uint16_t c) {
  i += c;
}

void Parser::skip_ws() {
  while (peek() == ' ' || peek() == '\t')
    advance();
}

void Parser::locator(AddressPromise &addr) {
  addr.base = AddressPromise::None{};
  switch (peek()) {
  case '.':
    advance();
    addr.base = AddressPromise::Current{};
    break;
  case '$':
    advance();
    addr.base = AddressPromise::Last{};
    break;
  case '%':
    advance();
    addr.base = AddressPromise::LastRange{};
    break;
  case '[':
    advance();
    addr.base = AddressPromise::Block{Direction::Backward};
    break;
  case ']':
    advance();
    addr.base = AddressPromise::Block{Direction::Forward};
    break;
  case '^':
    advance();
    addr.base = AddressPromise::Diagnostic{Direction::Backward};
    break;
  case '~':
    advance();
    addr.base = AddressPromise::Diagnostic{Direction::Forward};
    break;
  case '\'':
    advance();
    if (('a' <= peek() && peek() <= 'z')
        || ('A' <= peek() && peek() <= 'Z'))
      addr.base = AddressPromise::Mark{peek()};
    else
      throw ed_error("Valid mark needed after \'");
    advance();
    break;
  case '{': {
    advance();
    uint16_t j = 0;
    std::string func;
    std::string arg;
    while (peek(j) != '}') {
      if (peek(j) == '\0')
        throw ed_error("Scripted address not terminated");
      if (peek(j) == '\\')
        ++j;
      if (peek(j) == ':') {
        func = peek_str(j);
        advance(j + 1);
        j = 0;
        continue;
      }
      ++j;
    }
    if (func.size())
      arg = peek_str(j);
    else
      func = peek_str(j);
    addr.base = AddressPromise::Scripted{std::move(func), std::move(arg)};
    advance(j + 1);
  } break;
  case '/': {
    advance();
    uint64_t j = 0;
    while (true) {
      if (peek(j) == '\0')
        break;
      if (peek(j) == '/')
        break;
      else if (peek(j) == '\\')
        j += 2;
      else if (peek(j) == '[')
        while (peek(j) != '\0' && cmd[i] != ']')
          j++;
      else
        j++;
    }
    addr.base = AddressPromise::Regex(
      Direction::Forward,
      std::string(peek_str(j))
    );
    advance(j + 1);
  } break;
  case '?': {
    advance();
    uint16_t j = 0;
    while (true) {
      if (peek(j) == '\0')
        break;
      if (peek(j) == '?')
        break;
      else if (peek(j) == '\\')
        j += 2;
      else if (peek(j) == '[')
        while (peek(j) != '\0' && cmd[i] != ']')
          j++;
      else
        j++;
    }
    addr.base = AddressPromise::Regex(
      Direction::Backward,
      std::string(peek_str(j))
    );
    advance(j + 1);
  } break;
  case '<': {
    advance();
    uint16_t j = 0;
    while (peek(j) != '>'
           && peek(j) != '<'
           && peek(j) != '\0')
      j++;
    switch (peek(j)) {
    case '\0':
    case '>':
      addr.base = AddressPromise::SymbolDefinition{
        std::string(peek_str(j))
      };
      break;
    case '<':
      addr.base = AddressPromise::SymbolReference{
        Direction::Backward,
        std::string(peek_str(j))
      };
      break;
    }
    advance(j + 1);
  } break;
  case '>': {
    advance();
    uint16_t j = 0;
    while (peek(j) != '>' && peek(j) != '\0')
      j++;
    switch (peek(j)) {
    case '\0':
      throw ed_error("Unterminated symbol reference addressing.");
    case '>':
      addr.base = AddressPromise::SymbolReference{
        Direction::Forward,
        std::string(peek_str(j))
      };
      break;
    }
    advance(j + 1);
  } break;
  case '+': {
    advance();
    addr.base = AddressPromise::Current{};
    uint16_t j = 0;
    uint64_t num = 0;
    while ('0' <= peek(j) && peek(j) <= '9') {
      num = num * 10 + (peek(j) - '0');
      j++;
    }
    if (j == 0)
      num = 1;
    advance(j);
    addr.offset += num;
  } break;
  case '-': {
    advance();
    addr.base = AddressPromise::Current{};
    uint16_t j = 0;
    uint64_t num = 0;
    while ('0' <= peek(j) && peek(j) <= '9') {
      num = num * 10 + (peek(j) - '0');
      j++;
    }
    if (j == 0)
      num = 1;
    advance(j);
    addr.offset -= num;
  } break;
  default:
    if ('0' <= peek() && peek() <= '9') {
      uint64_t num = 0;
      while ('0' <= peek() && peek() <= '9') {
        num = num * 10 + (peek() - '0');
        advance();
      }
      addr.base = AddressPromise::Number{num};
    }
  }
}

int64_t Parser::offset() {
  int64_t offset = 0;
  while (peek() == '+' || peek() == '-'
         || ('0' <= peek() && peek() <= '9')) {
    bool positive = peek() != '-';
    if (peek() == '+' || peek() == '-')
      advance();
    uint16_t j = 0;
    int64_t num = 0;
    while ('0' <= peek(j) && peek(j) <= '9')
      num = num * 10 + (peek(j++) - '0');
    if (j == 0)
      num = 1;
    advance(j);
    offset += positive ? num : -num;
    skip_ws();
  }
  skip_ws();
  return offset;
}

void Parser::address(AddressPromise &addr) {
  if (peek() == ':') {
    advance();
    uint16_t j = 0;
    while (peek(j) != ':' && peek(j) != '\0')
      j++;
    addr.bufname = peek_str(j);
    advance(j);
    if (peek() == ':')
      advance();
  }
  skip_ws();
  if (peek() == '\0')
    return;
  locator(addr);
  skip_ws();
  addr.offset += offset();
}

void Parser::addresses(std::vector<AddressPromise> &addresses) {
  skip_ws();
  addresses.push_back({});
  auto *addr = &addresses.back();
  address(*addr);
  while (peek() == ',' || peek() == ';') {
    addr->jumping = peek() == ';';
    advance();
    skip_ws();
    addresses.push_back({});
    addr = &addresses.back();
    address(*addr);
  }
  if (addresses.size() == 1
      && addr->offset == 0 && addr->bufname.has_value()
      && std::holds_alternative<AddressPromise::None>(addr->base))
    addresses.pop_back();
}

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

void Parser::parse() {
  skip_ws();
  if (peek() == '@') {
    advance();
    command->temp_address = true;
  } else {
    command->temp_address = false;
  }
  skip_ws();
  addresses(command->addresses);
  operation();
  skip_ws();
  if (peek() != '\0')
    throw ed_error("Malformed command");
}

Parser::Parser(
  std::string_view cmd, BEd &bed, Command *command,
  std::vector<io::Token> *tokens, CompletionContext *completion
) : bed(bed), cmd(cmd), command(command), tokens(tokens), completion(completion) {
  i = 0;
}

Command Parser::get_command(std::string_view cmd, BEd &bed) {
  Command c;
  std::vector<io::Token> tokens;
  CompletionContext completion;
  Parser p(cmd, bed, &c, &tokens, &completion);
  p.parse();
  return c;
}

std::vector<AddressPromise> Parser::get_addresses(std::string_view cmd, BEd &bed) {
  std::vector<AddressPromise> result;
  std::vector<io::Token> tokens;
  CompletionContext completion;
  Parser p(cmd, bed, nullptr, &tokens, &completion);
  p.addresses(result);
  p.skip_ws();
  if (p.peek() != '\0')
    throw ed_error("Malformed address");
  return result;
}
} // namespace bed::internal::parser
