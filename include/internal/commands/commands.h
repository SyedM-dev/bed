#pragma once

#include "definitions.h"
#include "internal/trie/trie.h"
#include "pch.h"

namespace bed::internal::commands {
struct Command {
  enum struct AddressMode : uint8_t {
    None,
    Single,
    Range
  } address_mode;

  enum struct SuffixKind : uint8_t {
    None,
    Suffix,
    Argument,
    Continuation
  } suffix;

  std::string desc;

  bool accept_zero;

  void (*handle)(BEd &, std::span<const uint64_t>, std::string_view);

  static void register_posix(BEd &ctx);
};
} // namespace bed::internal::commands
