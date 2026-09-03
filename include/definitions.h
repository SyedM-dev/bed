#pragma once

#include "internal/vase/vase.h"
#include "pch.h"

namespace bed {
struct BEd;

struct fatal_error : std::runtime_error {
  uint8_t code;
  fatal_error(std::string msg, uint8_t code)
      : std::runtime_error(msg), code(code) {}
};

struct ed_error : std::runtime_error {
  ed_error(std::string msg) : std::runtime_error(msg) {}
};
} // namespace bed
