#include "internal/syntax/ruby/parser.h"

namespace bed::internal::syntax::ruby {
Language lang_ruby() {
  return Language{
    .none_state = []() {
      RubyState *st = (RubyState *)malloc(
        sizeof(RubyState)
        + sizeof(RubyState::RubyInternalState)
      );
      st->top = 1;
      st->docs = 0;
      st->stack()[0] = {
        .brace_level = 1,
        .lit_brace_level = 0,
        .state = RubyState::RubyInternalState::NONE,
        .flags = RubyState::RubyInternalState::EXPECTING_EXPRESSION,
        .delim_start = '\0',
        .delim_end = '\0'
      };
      return st; },
    .parse = ruby_parse,
    .copy = [](void *v_i_st) {
      RubyState *i_st = (RubyState *)v_i_st;
      uint32_t bytes = sizeof(RubyState)
                       + sizeof(RubyState::RubyInternalState) * i_st->top
                       + i_st->docs;
      RubyState *o_st = (RubyState *)malloc(bytes);
      memcpy(o_st, i_st, bytes);
      return o_st; },
    .equal = [](void *v_a_st, void *v_b_st) {
      RubyState *a_st = (RubyState *)v_a_st;
      uint32_t a_bytes = sizeof(RubyState)
                         + sizeof(RubyState::RubyInternalState) * a_st->top
                         + a_st->docs;
      RubyState *b_st = (RubyState *)v_b_st;
      uint32_t b_bytes = sizeof(RubyState)
                         + sizeof(RubyState::RubyInternalState) * b_st->top
                         + b_st->docs;
      return a_bytes == b_bytes && memcmp(a_st, b_st, a_bytes) == 0; },
    .destroy = [](void *v_st) { free(v_st); },
  };
}
} // namespace bed::internal::syntax::ruby
