#pragma once

#include "../shard.h"
#include "pch.h"

enum struct Direction : uint8_t {
  Forward,
  Backward
};

struct ChunkIterator {
  Direction dir;
  Shard *root = nullptr;
  std::vector<Shard *> stack;
  Petal *petal = nullptr;
  uint64_t petal_offset = 0;
  uint64_t global_offset = 0;
  uint64_t global_line = 0;
  bool at_end = false;

  ChunkIterator(Shard *r, Direction dir);

  ~ChunkIterator();

  void seek_offset(uint64_t offset);
  void seek_line(uint64_t offset);
  bool next(const char **data, uint64_t *out_len);
  uint64_t byte_offset();
  uint64_t line_offset();
};
