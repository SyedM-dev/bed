#include "internal/theme/theme.h"

namespace bed::internal::theme {
Theme::Theme() {
  hl.fill({
    .fg = 0xF0F0F0,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  });
}

io::Highlight Theme::get(const io::Token::Kind &token) const {
  return hl[token];
}

Theme Theme::default_theme() {
  Theme theme;
  theme.hl[io::Token::TempCurrent] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::BufferName] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::AddressSeperator] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Address] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Offset] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::AddressRegex] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::AddressSymbol] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Mark] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::RubyFunction] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::RubyArg] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Any] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Shell] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Ruby] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::File] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Regex] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Replacement] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Suffix] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color1] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color2] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color3] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color4] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color5] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Warning] = {
    .fg = 0xF1C55A,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  // Data is terminal default.
  theme.hl[io::Token::Shebang] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Error] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Comment] = {
    .fg = 0xAAAAAA,
    .bg = 0x000000,
    .flags = io::Highlight::Italic,
  };
  theme.hl[io::Token::String] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Escape] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Interpolation] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Regexp] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Number] = {
    .fg = 0xE6C08A,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::True] = {
    .fg = 0x7AE93C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::False] = {
    .fg = 0xEF5168,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Char] = {
    .fg = 0xFFAF70,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Keyword] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::KeywordOperator] = {
    .fg = 0xF07178,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Operator] = {
    .fg = 0xFFFFFF,
    .bg = 0x000000,
    .flags = io::Highlight::Italic,
  };
  theme.hl[io::Token::Function] = {
    .fg = 0xFFAF70,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Type] = {
    .fg = 0xF07178,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Constant] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::VariableInstance] = {
    .fg = 0x95E6CB,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::VariableGlobal] = {
    .fg = 0xF07178,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Annotation] = {
    .fg = 0x7DCFFF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Directive] = {
    .fg = 0xFF8F40,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Label] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace1] = {
    .fg = 0xD2A6FF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace2] = {
    .fg = 0xFFAFAF,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace3] = {
    .fg = 0xFFFF00,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace4] = {
    .fg = 0x0FFF0F,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace5] = {
    .fg = 0xFF0F0F,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  return theme;
}

Theme Theme::from_name(std::string_view name) {
  if (name == "default")
    return default_theme();
  throw ed_error("Unknown theme: " + std::string(name));
}
} // namespace bed::internal::theme
