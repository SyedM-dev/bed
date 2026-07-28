#pragma once
#include "pch.h"
#include "shard.h"

struct LineIterator {
  Shard *root = nullptr;
  std::vector<Branch *> stack;
  Petal *petal = nullptr;
  uint32_t petal_offset = 0;

  LineIterator(Shard *r, uint32_t start_line);

  ~LineIterator();

  LineIterator(const LineIterator &) = delete;
  LineIterator &operator=(const LineIterator &) = delete;
  LineIterator(LineIterator &&) = default;
  LineIterator &operator=(LineIterator &&) = default;

  void seek_line(uint32_t line_index);
  bool next(std::string &line);

private:
  void descend(Shard *s);
  bool advance_petal();
};
