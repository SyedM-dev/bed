#pragma once

#include "definitions.h"
#include "internal/syntax/parser.h"
#include "pch.h"

namespace bed::internal::theme {
struct Highlight {
  enum : uint8_t {
    None = 0,
    Bold = 1 << 0,
    Italic = 1 << 1,
    Strikethrough = 1 << 2,
    Underline = 1 << 3,
  };
  uint32_t fg;
  uint32_t bg;
  uint8_t flags;
};

struct Theme {
  std::array<Highlight, internal::syntax::Token::Count> hl;

  Theme();

  Highlight get(internal::syntax::Token token) const;

  static Theme default_theme();
  static Theme from_name(std::string_view name);
};
} // namespace bed::internal::theme
