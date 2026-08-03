#pragma once

#include "buffer/append.h"
#include "buffer/original.h"
#include "constants.h"
#include "iterators/line.h"
#include "pch.h"
#include "search.h"
#include "shard.h"
#include "utils/utils.h"

struct Point {
  uint32_t row;
  uint32_t col;
};

struct Range {
  Point start;
  Point end;
};

struct Vase {
  OriginalBuffer original;
  AppendBuffer append;
  Shard *root;

  Vase(std::string path);

  ~Vase();

  uint32_t length();
  std::string to_string();

  Point resolve_column(uint32_t line, uint32_t column);

  LineIterator iterate(uint32_t line);

  void insert(Point *point, char key);
  void insert(Point *point, const char *data, uint32_t len);
  void erase(Point *point, int64_t amount);
  void erase(Range range);
  void replace(Range range, const char *data, uint32_t len);

  void move_clusters(Point *point, int64_t amount);
  void move_lines(Point *point, int64_t amount);
  void clamp(Point *point);

  void regex_search_replace(
    std::string_view pattern, Range range,
    std::string_view replace, std::string_view options
  );

  bool undo();
  bool redo();

  void snapshot();
  void prune_history(uint32_t n);

private:
  std::vector<Shard *> history;
  uint32_t history_top;

  uint32_t offset_of(Point point);
  static void flatten(Shard *s, std::string &out);
  void _insert(Point *point, const char *data, uint32_t len);
};
