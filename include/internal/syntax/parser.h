#pragma once

#include "decl.h"
#include "pch.h"

namespace bed::internal::syntax {
struct ParserSnapshot {
  ParseState *root = nullptr;
  Language *lang = nullptr;
};

ParserSnapshot make_parser(vase::Shard *vase, uint64_t lines, Language *lang);
ParserSnapshot retain(const ParserSnapshot &snap);
void release(ParserSnapshot &snap);

uint64_t next_closing(const ParserSnapshot &snap, uint64_t line);
uint64_t prev_opening(const ParserSnapshot &snap, uint64_t line);

struct Iterator {
  ParserSnapshot snap;
  std::optional<vase::Iterator> it;
  void *state;
  uint64_t at;
  std::vector<io::Token> tokens;
  std::vector<ParseEvent> events;
  Iterator(uint64_t, ParserSnapshot, vase::Shard *);
  ~Iterator();
  Iterator(const Iterator &) = delete;
  Iterator &operator=(const Iterator &) = delete;
  Iterator(Iterator &&other);
  Iterator &operator=(Iterator &&other);
  void next();
};

std::optional<Iterator> get_hl(const ParserSnapshot &snap, vase::Shard *vase, uint64_t target);

void insert(ParserSnapshot &snap, vase::Shard *vase, uint64_t start, uint64_t count);
void erase(ParserSnapshot &snap, vase::Shard *vase, uint64_t start, uint64_t count);

struct Edit {
  ParserSnapshot *target;
  bool dirty = false;
  uint64_t dirty_start = 0;
  uint64_t dirty_end = 0;
  int64_t edit_delta = 0;

  explicit Edit(ParserSnapshot &snap) : target(&snap) {}

  void mark_dirty(uint64_t start, uint64_t end);
  void insert(uint64_t start, uint64_t count);
  void erase(uint64_t start, uint64_t count);
  void commit(vase::Shard *vase);
};
} // namespace bed::internal::syntax
