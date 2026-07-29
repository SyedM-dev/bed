#pragma once

#include "../shard.h"
#include "chunk.h"
#include "pch.h"

struct LineIterator {
  uint32_t offset;
  ChunkIterator it;

  const char *cursor = nullptr;
  uint32_t remaining = 0;
  bool eof = false;

  LineIterator(Shard *r, uint32_t start_line);

  ~LineIterator();

  bool next(std::string &line);
};
