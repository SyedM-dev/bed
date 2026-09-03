#pragma once

#include "internal/functions/functions.h"
#include "internal/functions/suffixes.h"
#include "internal/ui/command.h"
#include "pch.h"

namespace bed::internal::parser {
struct AddressPromise {
  struct None {};    //
  struct Current {}; // .
  struct Last {};    // $
  struct Number {    // n
    uint64_t i;
  };
  struct Mark { // 'm
    char m;
  };
  struct Regex { // /re/ or ?re?
    internal::Direction dir;
    std::string re;
  };
  struct Diagnostic { // ^ or ~ for previous/next diagnostic
    internal::Direction dir;
  };
  struct SymbolDefinition { // <sym> goto symbol definition. (From lsp or using internal language parsers)
    std::string sym;
  };
  struct SymbolReference {   // >sym> or <sym<
    internal::Direction dir; // goto next or previous symbol reference
    std::string sym;
  };
  struct Block { // [ or ] goto start/end of containing block.
    internal::Direction dir;
  };
  struct Scripted {   // {function_name:arguments}
    std::string func; // Calls ruby mapping with name giving the argument as string.
    std::string arg;  // resolves to line number returned by function (or throws error).
                      // The mruby runtime also has the full context of the file and extentions etc.
  };
  struct LastRange {}; // % , refers to last used range

  std::optional<std::string> bufname;
  std::variant<
    None,
    Current,
    Last,
    Number,
    Mark,
    Regex,
    Diagnostic,
    SymbolDefinition,
    SymbolReference,
    Block,
    Scripted,
    LastRange>
    base{};
  int64_t offset = 0;
  bool jumping{false}; // ; == jumping and , == non jumping
  buffer::Line resolve(BEd &ctx);
  static std::optional<buffer::Line> get_line(BEd &ctx, std::vector<AddressPromise> &);
  static std::optional<buffer::Range> get_range(BEd &ctx, std::vector<AddressPromise> &);
};

struct Command {
  bool temp_address{false};
  std::vector<AddressPromise> addresses{};
  functions::Function *function{nullptr};
  functions::Function::Argument argument{std::monostate()};
  std::vector<AddressPromise> argument_addresses{};
  functions::Suffix *suffix{nullptr};
};

struct CompletionContext {
  enum {
    Error,
    Valid,
    Incomplete
  } error;
  // TODO: store a state of typing context to think about possible completions.
};

struct Parser {
  BEd &bed;
  std::string_view cmd;
  uint16_t i;
  Command *command;
  std::vector<io::Token> *tokens;
  CompletionContext *completion;

  explicit Parser(
    std::string_view cmd, BEd &bed, Command *command,
    std::vector<io::Token> *tokens, CompletionContext *completion
  );

  char peek(uint16_t = 0); // == \0 if at eof.
  std::string_view peek_str(uint16_t = UINT16_MAX);
  void advance(uint16_t = 1);

  void skip_ws();

  void locator(AddressPromise &);
  int64_t offset();
  void address(AddressPromise &addr);
  void addresses(std::vector<AddressPromise> &addresses);

  void operation();

  void parse();
  static std::vector<io::Token> get_highlight(std::string_view cmd, BEd &bed);
  static Command get_command(std::string_view, BEd &);
  static std::vector<AddressPromise> get_addresses(std::string_view cmd, BEd &bed);
};
} // namespace bed::internal::parser
