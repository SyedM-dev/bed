#pragma once

#include "definitions.h"
#include "internal/buffer/buffer.h"
#include "pch.h"

namespace bed::internal::functions {
struct Suffix {
  std::string desc;
  std::function<void(BEd &)> handle;

  static void register_suffixes(BEd &ctx);
};
} // namespace bed::internal::functions
