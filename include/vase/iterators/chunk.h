#pragma once

#include "../shard.h"
#include "pch.h"

struct ChunkIterator {
  Shard *root = nullptr;
  std::vector<Shard *> stack;
  Petal *petal = nullptr;
  uint32_t petal_offset = 0;
  uint32_t global_offset;

  ChunkIterator(Shard *r);

  ~ChunkIterator();

  void seek_offset(uint32_t offset);
  void seek_line(uint32_t offset);
  bool next(const char **data, uint32_t *out_len);
  uint32_t byte_offset();
};
