#pragma once

#include "../shard.h"
#include "chunk.h"
#include "pch.h"

struct LineIterator {
  ChunkIterator it;
  Direction dir;

  const char *chunk;
  uint64_t len;
  uint64_t offset = 0;
  uint64_t chunk_offset = 0;
  bool at_end = false;

  LineIterator(Shard *r, uint64_t start_line, Direction dir);
  ~LineIterator() = default;

  bool next(std::string &line);
  uint64_t byte_offset();
};
