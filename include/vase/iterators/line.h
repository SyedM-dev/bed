#pragma once

#include "../shard.h"
#include "chunk.h"
#include "pch.h"

struct LineIterator {
  ChunkIterator it;

  const char *cursor = nullptr;
  bool eof = false;
  uint32_t remaining = 0;
  uint32_t line_offset = 0;

  LineIterator(Shard *r, uint32_t start_line);

  ~LineIterator() = default;

  bool next(std::string &line);
  uint32_t byte_offset();
};
