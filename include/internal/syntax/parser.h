#pragma once

#include "decl.h"
#include "internal/vase/vase.h"
#include "pch.h"

namespace bed::internal::syntax {
struct Parser {
  static constexpr uint64_t MAX_CHUNK = 512;

  ParseState *root;
  Language lang;
  bool in_edit = false;
  bool dirty = false;
  uint64_t dirty_start = 0;
  uint64_t dirty_end = 0;

  Parser(vase::Shard *, uint64_t, Language);
  ~Parser();
  Parser(const Parser &) = delete;
  Parser &operator=(const Parser &) = delete;

  void reset(vase::Shard *, uint64_t, Language);
  void erase(vase::Shard *, uint64_t, uint64_t);
  void insert(vase::Shard *, uint64_t, uint64_t);
  void modify(vase::Shard *, uint64_t, uint64_t);

  void begin_edit();
  void erase(uint64_t start, uint64_t count);
  void insert(uint64_t start, uint64_t count);
  void end_edit(vase::Shard *vase);
  void mark_dirty(uint64_t start, uint64_t end);

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
    Iterator(uint64_t, Parser *, vase::Shard *);
    ~Iterator();
    Iterator(const Iterator &) = delete;
    Iterator &operator=(const Iterator &) = delete;
    Iterator(Iterator &&other);
    Iterator &operator=(Iterator &&other);
    void next();
  };
  std::optional<Iterator> get_hl(vase::Shard *, uint64_t);
};
} // namespace bed::internal::syntax
