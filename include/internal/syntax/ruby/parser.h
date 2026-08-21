#pragma once

#include "decl.h"
#include "pch.h"

namespace bed::internal::syntax::ruby {
struct RubyParser {
  void **v_state;
  RubyState *state;
  std::string_view line;
  uint32_t i = 0;
  bool heredoc_start_line = false;
  bool ending = true;
  bool op_last = false;
  bool set_op_last = false;

  RubyParser(void **v_state, std::string_view line)
      : v_state(v_state), state((RubyState *)*v_state), line(line) {}

  uint32_t len() const {
    return line.size();
  }

  RubyState::RubyInternalState &current() {
    return state->stack()[state->top - 1];
  }

  char peek(int32_t offset = 0) {
    uint32_t pos = i + offset;
    return pos < line.size() ? line[pos] : '\0';
  }

  std::string_view peek_str(uint32_t len) {
    return line.substr(i, len);
  }

  void advance() {
    ++i;
  }

  void advance(uint32_t n) {
    i += n;
  }

  void push_state() {
    state->top++;
    *v_state = (RubyState *)realloc(
      *v_state,
      sizeof(RubyState)
        + sizeof(RubyState::RubyInternalState) * state->top
        + state->docs
    );
    state = (RubyState *)*v_state;
    memmove(
      state->heredocs(),
      state->stack() + state->top - 1,
      state->docs
    );
    current() = {
      .brace_level = 1,
      .lit_brace_level = 0,
      .state = RubyState::RubyInternalState::NONE,
      .flags = RubyState::RubyInternalState::EXPECTING_EXPRESSION | RubyState::RubyInternalState::NEWLINE,
      .delim_start = '\0',
      .delim_end = '\0'
    };
  }

  void pop_state() {
    state->top--;
    memmove(
      state->heredocs(),
      state->stack() + state->top + 1,
      state->docs
    );
  }

  void enqueue_doc(uint8_t header, std::string_view name) {
    uint32_t bytes = 1 + name.size();
    uint32_t old_docs = state->docs;
    state->docs += bytes;
    *v_state = (RubyState *)realloc(
      *v_state,
      sizeof(RubyState)
        + sizeof(RubyState::RubyInternalState) * state->top
        + state->docs
    );
    state = (RubyState *)*v_state;
    uint8_t *heredocs = state->heredocs();
    heredocs[old_docs] = header;
    memcpy(heredocs + old_docs + 1, name.data(), name.size());
  }

  bool dequeue_doc(uint8_t heredoc_len) {
    uint32_t bytes = heredoc_len + 1;
    uint8_t *heredocs = state->heredocs();
    if (state->docs -= bytes)
      memmove(heredocs, heredocs + bytes, state->docs);
    else
      return false;
    return true;
  }
};

void ruby_parse(
  void **v_state, std::string_view line, bool first_line,
  std::vector<Token> *tokens, std::vector<ParseEvent> *events
);
} // namespace bed::internal::syntax::ruby
