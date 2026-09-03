#pragma once

#include "definitions.h"
#include "internal/syntax/parser.h"
#include "pch.h"

namespace bed::internal::theme {
struct Theme {
  std::array<io::Highlight, io::Token::Count> hl;

  Theme();

  io::Highlight get(const io::Token::Kind &token) const;

  static Theme default_theme();
  static Theme from_name(std::string_view name);
};
} // namespace bed::internal::theme
