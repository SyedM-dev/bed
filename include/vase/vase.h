#pragma once

#include "buffer/append.h"
#include "buffer/original.h"
#include "iterators/chunk.h"
#include "iterators/line.h"
#include "pch.h"
#include "search.h"
#include "shard.h"

struct Vase {
  OriginalBuffer original;
  AppendBuffer append;

  /*std::vector<Shard *> undo; // TODO: later
  uint8_t top; // of the undo stack.
  uint8_t max; // for redo when no edits have been done after some undo.*/

  Shard *root;

  Vase(char *data, uint32_t length);

  ~Vase();

  uint32_t length();

  std::string to_string();
  static void flatten(Shard *s, std::string &out);

  void type(uint32_t offset, char key);
  void insert(uint32_t offset, const char *data, uint32_t len);
  void erase(uint32_t cursor, int64_t amount);

  // Grapheme clusters or the specail cluster "\r\n'
  void erase_clusters(uint32_t cursor, int64_t amount);

  // moves in clusters from line,col and sets new position to line,col returns false if not enough space to jump.
  bool jump_clusters(uint32_t *line, uint32_t *col, int64_t amount);

  // need to also make helpers for line width.

  void regex_search_replace(
    std::string_view pattern,
    uint32_t start_offset, uint32_t end_offset,
    std::string_view replace, std::string_view options
  );
};
