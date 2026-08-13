#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::commands {
struct Suffix {
  std::string desc;
  void (*handle)(BEd &);

  static void register_suffixes(BEd &ctx);
};
} // namespace bed::internal::commands
