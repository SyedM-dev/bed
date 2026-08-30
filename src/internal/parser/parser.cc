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
  std::vector<ui::Token> *tokens, CompletionContext *completion
) : bed(bed), cmd(cmd), command(command), tokens(tokens), completion(completion) {
  i = 0;
}

Command Parser::get_command(std::string_view cmd, BEd &bed) {
  Command c;
  std::vector<ui::Token> tokens;
  CompletionContext completion;
  Parser p(cmd, bed, &c, &tokens, &completion);
  p.parse();
  return c;
}

std::vector<AddressPromise> Parser::get_addresses(std::string_view cmd, BEd &bed) {
  std::vector<AddressPromise> result;
  std::vector<ui::Token> tokens;
  CompletionContext completion;
  Parser p(cmd, bed, nullptr, &tokens, &completion);
  p.addresses(result);
  p.skip_ws();
  if (p.peek() != '\0')
    throw ed_error("Malformed address");
  return result;
}
} // namespace bed::internal::parser
