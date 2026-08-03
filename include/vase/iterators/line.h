#pragma once

#include "../shard.h"
#include "chunk.h"
#include "pch.h"

struct LineIterator {
  ChunkIterator it;
  Direction dir;

  const char *chunk;
  uint32_t len;

  LineIterator(Shard *r, uint32_t start_line, Direction dir);
  ~LineIterator() = default;

  bool next(std::string &line);
  uint32_t byte_offset();
};
