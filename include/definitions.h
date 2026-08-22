#pragma once

#include "internal/vase/vase.h"
#include "pch.h"

namespace bed {
struct fatal_error : std::runtime_error {
  uint8_t code;
  fatal_error(std::string msg, uint8_t code)
      : std::runtime_error(msg), code(code) {}
};

struct ed_error : std::runtime_error {
  ed_error(std::string msg) : std::runtime_error(msg) {}
};

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

struct BEd;
} // namespace bed
