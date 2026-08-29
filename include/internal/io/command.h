#pragma once

#include "internal/io/io.h"
#include "internal/ui/autocomp.h"
#include "pch.h"

namespace bed::internal::io {
struct Token {
  enum struct Type : uint8_t {
    TempCurrent,      // @
    AddressSeperator, // ; ,
    Address,          // . $ % [ ] ^ ~
    Offset,           // +N -N + -
    AddressRegex,     // /re/ ?re?
    AddressSymbol,    // >s> <s< <s>
    Number,           // 10
    Mark,             // 'm
    RubyFunction,     // (func:arg)
    RubyArg,
    Function,
    Any,
    Shell,
    Ruby,
    File,
    Regex,
    Replacement,
    Suffix
  } type;
  uint16_t start;
  uint16_t end;
};

struct CommandIO {
  std::string cmd;
  uint16_t cursor;
  std::string prompt;
  uint16_t start;
  uint16_t height;
  uint16_t term_height;
  uint16_t term_width;
  BEd &bed;
  IO &io;

  CommandIO(BEd &, IO &);
  std::pair<std::string, bool> run();
  void redraw();
};
} // namespace bed::internal::io
