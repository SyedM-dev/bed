#pragma once

#include "decl.h"
#include "internal/vase/vase.h"
#include "pch.h"

namespace bed::internal::syntax {
void dump_events(ParseState *node);

struct Parser {
  static constexpr uint64_t MAX_CHUNK = 512;

  ParseState *root;
  Language lang;

  Parser(vase::Vase &, uint64_t, Language);
  ~Parser();
  Parser(const Parser &) = delete;
  Parser &operator=(const Parser &) = delete;

  void reset(vase::Vase &, uint64_t, Language);
  void erase(vase::Vase &, uint64_t, uint64_t);
  void insert(vase::Vase &, uint64_t, uint64_t);
  void modify(vase::Vase &, uint64_t, uint64_t);

  uint64_t next_closing(uint64_t line);
  uint64_t prev_opening(uint64_t line);

  std::pair<ParseState *, ParseState *> split_tree(ParseState *node, uint64_t line);
  ParseState *join_tree(ParseState *a, ParseState *b);

  struct Iterator {
    Parser *p;
    std::optional<vase::Iterator> it;
    void *state;
    uint64_t at;
    std::vector<Token> tokens;
    std::vector<ParseEvent> events;
    Iterator(uint64_t, Parser *, vase::Vase &);
    ~Iterator();
    Iterator(const Iterator &) = delete;
    Iterator &operator=(const Iterator &) = delete;
    Iterator(Iterator &&other);
    Iterator &operator=(Iterator &&other);
    void next();
  };
  std::optional<Iterator> get_hl(vase::Vase &, uint64_t);
};
} // namespace bed::internal::syntax
