#pragma once

#include "../decl.h"
#include "pch.h"
#include "tries.h"

namespace bed::internal::syntax::ruby {
struct alignas(2) RubyState {
  struct RubyInternalState {
    uint16_t brace_level;
    uint16_t lit_brace_level;
    enum : uint8_t {
      NONE,
      STRING,
      REGEXP,
      HEREDOC,
      COMMENT,
      END
    } state;
    static constexpr const uint8_t EXPECTING_EXPRESSION = 0b00010000;
    static constexpr const uint8_t ALLOW_INTERPOLATION = 0b00000001;
    uint8_t flags;
    char delim_start;
    char delim_end;
  };

  struct Heredocs {
    // header masks
    static constexpr const uint8_t ALLOW_INDENTATION = 0b10000000;
    static constexpr const uint8_t ALLOW_INTERPOLATION = 0b01000000;
    static constexpr const uint8_t LEN_MASK = 0b00111111;
    // the rest will be len number of bytes with the actual name.
  };

  uint8_t top;
  uint8_t docs;
  // the stack (RubyInternalState * top)
  // the docs queue (docs)

  inline RubyInternalState *stack() {
    return (RubyInternalState *)(this + 1);
  }

  inline uint8_t *heredocs() {
    return (uint8_t *)(stack() + top);
  }
};

Language lang_ruby();
} // namespace bed::internal::syntax::ruby
