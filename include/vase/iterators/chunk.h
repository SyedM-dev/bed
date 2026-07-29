#pragma once

#include "../shard.h"
#include "pch.h"

struct ChunkIterator {
  Shard *root = nullptr;
  std::vector<Shard *> stack;
  Petal *petal = nullptr;
  uint32_t petal_offset = 0;

  ChunkIterator(Shard *r, uint32_t offset = 0);

  ~ChunkIterator();

  void seek(uint32_t offset);
  bool next(const char **data, uint32_t *out_len);
};
