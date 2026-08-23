#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::commands {
struct Suffix {
  std::string desc;
  void (*handle)(BEd &, std::span<const uint64_t>);

  static void register_suffixes(BEd &ctx);
};
} // namespace bed::internal::commands
