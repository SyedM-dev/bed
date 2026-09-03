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
  tokens->push_back(
    {.start = i,
     .end = i + len,
     .type = io::Token::Function}
  );
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
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::Mark}
    );
    advance();
    break;
  case functions::Function::ArgumentKind::Any:
    command->argument = std::string(peek_str());
    tokens->push_back(
      {.start = i,
       .end = i + peek_str().size(),
       .type = io::Token::Any}
    );
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
    if (!(peek() == '\t' || peek() == ' ' || peek() == '\0'))
      throw ed_error("Incorrect file input usage.");
    skip_ws();
    switch (peek()) {
    case '!':
      tokens->push_back(
        {.start = i,
         .end = (uint64_t)i + 1,
         .type = io::Token::Error}
      );
      advance();
      command->argument = functions::Function::ShellArg(std::string(peek_str()));
      tokens->push_back(
        {.start = i,
         .end = i + peek_str().size(),
         .type = io::Token::Shell}
      );
      advance(peek_str().size());
      break;
    case '\0':
      command->argument = std::monostate();
      break;
    default:
      command->argument = std::filesystem::path(peek_str());
      tokens->push_back(
        {.start = i,
         .end = i + peek_str().size(),
         .type = io::Token::File}
      );
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
    uint16_t start = i;
    advance();
    uint16_t j = 0;
    while (true) {
      if (peek(j) == '\0')
        throw ed_error("Unterminated regex");
      if (peek(j) == delim)
        break;
      else if (peek(j) == '\\')
        j += 2;
      else if (peek(j) == '[')
        while (peek(j) != '\0' && peek(j) != ']')
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
        while (peek(j) != '\0' && peek(j) != ']')
          j++;
      else
        j++;
    }
    std::string replacement(peek_str(j));
    std::string options;
    if (peek(j) != '\0') {
      advance(j + 1);
    } else {
      advance(j);
      options = "p";
    }
    tokens->push_back(
      {.start = start,
       .end = i,
       .type = io::Token::Regexp}
    );
    if (peek() != '\0') {
      options = std::string(peek_str());
      tokens->push_back(
        {.start = i,
         .end = i + peek_str().size(),
         .type = io::Token::Suffix}
      );
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
  case functions::Function::ArgumentKind::Ruby: {
    command->argument = functions::Function::RubyArg(std::string(peek_str()));
    auto *ruby_parser = bed.languages["ruby"];
    void *state = ruby_parser->none_state();
    std::vector<syntax::ParseEvent> events;
    std::vector<io::Token> tokens_;
    ruby_parser->parse(&state, peek_str(), false, &tokens_, &events);
    for (auto &token : tokens_) {
      token.start += i;
      token.end += i;
      tokens->push_back(token);
    }
    ruby_parser->destroy(state);
    advance(peek_str().size());
  } break;
  case functions::Function::ArgumentKind::Shell:
    command->argument = functions::Function::ShellArg(std::string(peek_str()));
    tokens->push_back(
      {.start = i,
       .end = i + peek_str().size(),
       .type = io::Token::Shell}
    );
    advance(peek_str().size());
    break;
  }
  if (!suffix) {
    suffix = peek();
    tokens->push_back(
      {.start = i,
       .end = i + (uint64_t)1,
       .type = io::Token::Suffix}
    );
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
