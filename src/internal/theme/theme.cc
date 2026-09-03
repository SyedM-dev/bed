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

  constexpr uint32_t PINK = 0xECA1AE;
  constexpr uint32_t GREY = 0xAFB7D3;
  constexpr uint32_t BLUE = 0x799EDB;
  constexpr uint32_t LIGHT_BLUE = 0xB5BFFE;
  constexpr uint32_t RED = 0xF38CAA;
  constexpr uint32_t YELLOW = 0xF9E3B1;
  constexpr uint32_t PURPLE = 0xA286C7;
  constexpr uint32_t CYAN = 0x89DBEA;
  constexpr uint32_t LIGHT_GREEN = 0x98CE94;
  constexpr uint32_t TURQUOISE = 0x00BDB7;
  constexpr uint32_t BRIGHT_GREEN = 0x4AF6CA;

  theme.hl[io::Token::TempCurrent] = {
    .fg = PINK,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::BufferName] = {
    .fg = PURPLE,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::AddressSeperator] = {
    .fg = GREY,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::AddressSymbol] = {
    .fg = BLUE,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Mark] = {
    .fg = LIGHT_BLUE,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::RubyFunction] = {
    .fg = RED,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::RubyArg] = {
    .fg = LIGHT_GREEN,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Any] = {
    .fg = GREY,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Shell] = {
    .fg = BRIGHT_GREEN,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::File] = {
    .fg = GREY,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Suffix] = {
    .fg = TURQUOISE,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color1] = {
    .fg = 0x7AA2F7,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color2] = {
    .fg = 0xAAD94C,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color3] = {
    .fg = 0xFF9E64,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color4] = {
    .fg = 0xBB9AF7,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Color5] = {
    .fg = 0xFF757F,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Warning] = {
    .fg = 0xF1C55A,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Data] = {
    .fg = GREY,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
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
    .fg = 0x7DCFFF,
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
    .fg = BRIGHT_GREEN,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace2] = {
    .fg = YELLOW,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace3] = {
    .fg = 0xFFFF00,
    .bg = 0x000000,
    .flags = io::Highlight::None,
  };
  theme.hl[io::Token::Brace4] = {
    .fg = CYAN,
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
