#pragma once

#include "buffer/append.h"
#include "buffer/original.h"
#include "iterators/line.h"
#include "pch.h"
#include "search.h"
#include "shard.h"
#include "utils/utils.h"

struct Vase {
  Vase(char *data, uint32_t length);

  ~Vase();

  uint32_t length();
  std::string to_string();

  Point resolve_column(uint32_t line, uint32_t column);

  LineIterator iterate(uint32_t line);

  void input(Point *point, char key);
  void insert(Point *point, const char *data, uint32_t len);
  void erase(Point *point, int64_t amount);
  bool jump(Point *point, int64_t amount);
  bool clamp(Point *point);
  void regex_search_replace(
    std::string_view pattern,
    Point start, Point end,
    std::string_view replace, std::string_view options
  );

private:
  OriginalBuffer original;
  AppendBuffer append;
  Shard *root;

  /*std::vector<Shard *> undo; // TODO: later
  uint16_t top; // of the undo stack.
  uint16_t max; // for redo when no edits have been done after some undo.*/

  uint32_t offset_of(Point point);
  static void flatten(Shard *s, std::string &out);
};
