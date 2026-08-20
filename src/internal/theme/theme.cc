#include "internal/theme/theme.h"

namespace bed::internal::theme {
Theme::Theme() {
  hl.fill({
    .fg = 0xF0F0F0,
    .bg = 0x000000,
    .flags = Highlight::None,
  });
}

Highlight Theme::get(internal::syntax::Token token) const {
  return hl[token.type];
}

Theme Theme::default_theme() {
  Theme theme;
  theme.hl[internal::syntax::Token::Shebang] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Error] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Comment] = {
    .fg = 0xAAAAAA,
    .bg = 0x000000,
    .flags = Highlight::Italic,
  };
  theme.hl[internal::syntax::Token::String] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Escape] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Interpolation] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Regexp] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Number] = {
    .fg = 0xE6C08A,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::True] = {
    .fg = 0x7AE93C,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::False] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Char] = {
    .fg = 0xFFAF70,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Keyword] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::KeywordOperator] = {
    .fg = 0xF07178,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Operator] = {
    .fg = 0xFFFFFF,
    .bg = 0x000000,
    .flags = Highlight::Italic,
  };
  theme.hl[internal::syntax::Token::Function] = {
    .fg = 0xFFAF70,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Type] = {
    .fg = 0xF07178,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Constant] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::VariableInstance] = {
    .fg = 0x95E6CB,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::VariableGlobal] = {
    .fg = 0xF07178,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Annotation] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Directive] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Label] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Brace1] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Brace2] = {
    .fg = 0xFFAFAF,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Brace3] = {
    .fg = 0xFFFF00,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Brace4] = {
    .fg = 0x0FFF0F,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  theme.hl[internal::syntax::Token::Brace5] = {
    .fg = 0xFF0F0F,
    .bg = 0x000000,
    .flags = Highlight::None,
  };
  return theme;
}

Theme Theme::from_name(std::string_view name) {
  if (name == "default")
    return default_theme();
  throw std::runtime_error("Unknown theme: " + std::string(name));
}

} // namespace bed::internal::theme
