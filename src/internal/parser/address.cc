#include "bed.h"
#include "internal/parser/parser.h"

namespace bed::internal::parser {
void Parser::locator(AddressPromise &addr) {
  addr.base = AddressPromise::None{};
  switch (peek()) {
  case '.':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::Current{};
    break;
  case '$':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::Last{};
    break;
  case '%':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::LastRange{};
    break;
  case '[':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::Block{Direction::Backward};
    break;
  case ']':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::Block{Direction::Forward};
    break;
  case '^':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::Diagnostic{Direction::Backward};
    break;
  case '~':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
    advance();
    addr.base = AddressPromise::Diagnostic{Direction::Forward};
    break;
  case '\'':
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)2,
       .type = io::Token::Mark}
    );
    advance();
    if (('a' <= peek() && peek() <= 'z')
        || ('A' <= peek() && peek() <= 'Z'))
      addr.base = AddressPromise::Mark{peek()};
    else
      throw ed_error("Valid mark needed after \'");
    advance();
    break;
  case '{': {
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
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
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)func.size(),
       .type = io::Token::RubyFunction}
    );
    if (arg.size())
      tokens->push_back(
        {.start = i + (uint64_t)func.size() + 1,
         .end = i + (uint64_t)func.size() + 1 + (uint64_t)arg.size(),
         .type = io::Token::RubyFunction}
      );
    addr.base = AddressPromise::Scripted{std::move(func), std::move(arg)};
    advance(j + 1);
    tokens->push_back(
      {.start = i,
       .end = i - (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
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
    tokens->push_back(
      {.start = (uint64_t)i - 1,
       .end = (uint64_t)i + j + 1,
       .type = io::Token::Regexp}
    );
    advance(j + 1);
  } break;
  case '?': {
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
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
    tokens->push_back(
      {.start = (uint64_t)i - 1,
       .end = (uint64_t)i + j + 1,
       .type = io::Token::Regexp}
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
    tokens->push_back(
      {.start = (uint64_t)i - 1,
       .end = (uint64_t)i + j + 1,
       .type = io::Token::AddressSymbol}
    );
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
    tokens->push_back(
      {.start = (uint64_t)i - 1,
       .end = (uint64_t)i + j + 1,
       .type = io::Token::AddressSymbol}
    );
    advance(j + 1);
  } break;
  case '+': {
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
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
    tokens->push_back(
      {.start = (uint64_t)i,
       .end = (uint64_t)i + j,
       .type = io::Token::Number}
    );
    advance(j);
    addr.offset += num;
  } break;
  case '-': {
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSymbol}
    );
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
    tokens->push_back(
      {.start = (uint64_t)i,
       .end = (uint64_t)i + j,
       .type = io::Token::Number}
    );
    advance(j);
    addr.offset -= num;
  } break;
  default:
    if ('0' <= peek() && peek() <= '9') {
      uint16_t start = i;
      uint64_t num = 0;
      while ('0' <= peek() && peek() <= '9') {
        num = num * 10 + (peek() - '0');
        advance();
      }
      addr.base = AddressPromise::Number{num};
      tokens->push_back(
        {.start = (uint64_t)start,
         .end = (uint64_t)i,
         .type = io::Token::Number}
      );
    }
  }
}

int64_t Parser::offset() {
  int64_t offset = 0;
  while (peek() == '+' || peek() == '-'
         || ('0' <= peek() && peek() <= '9')) {
    bool positive = peek() != '-';
    if (peek() == '+' || peek() == '-') {
      tokens->push_back(
        {.start = i,
         .end = i + (uint64_t)1,
         .type = io::Token::AddressSymbol}
      );
      advance();
    }
    uint16_t j = 0;
    int64_t num = 0;
    while ('0' <= peek(j) && peek(j) <= '9')
      num = num * 10 + (peek(j++) - '0');
    if (j == 0)
      num = 1;
    tokens->push_back(
      {.start = (uint64_t)i,
       .end = (uint64_t)i + j,
       .type = io::Token::Number}
    );
    advance(j);
    offset += positive ? num : -num;
    skip_ws();
  }
  skip_ws();
  return offset;
}

void Parser::address(AddressPromise &addr) {
  if (peek() == ':') {
    uint16_t start = i;
    advance();
    uint16_t j = 0;
    while (peek(j) != ':' && peek(j) != '\0')
      j++;
    addr.bufname = peek_str(j);
    advance(j);
    if (peek() == ':')
      advance();
    tokens->push_back(
      {.start = start,
       .end = i,
       .type = io::Token::BufferName}
    );
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
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::AddressSeperator}
    );
    advance();
    skip_ws();
    addresses.push_back({});
    addr = &addresses.back();
    address(*addr);
  }
  if (addresses.size() == 1
      && addr->offset == 0 && !addr->bufname.has_value()
      && std::holds_alternative<AddressPromise::None>(addr->base))
    addresses.pop_back();
}
} // namespace bed::internal::parser
