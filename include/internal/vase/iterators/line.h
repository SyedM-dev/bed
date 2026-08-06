#pragma once

#include "../shard.h"
#include "pch.h"
#include "petal.h"

namespace crib::internal::vase {
struct LineIterator {
  PetalIterator it;
  const char *chunk;
  uint64_t len;
  Direction dir;
  uint64_t current_offset = 0;
  uint64_t last_line_offset = 0;

  LineIterator(Shard *root, uint64_t line_num, Direction dir);
  ~LineIterator() = default;

  bool next(std::string *line);
  uint64_t byte_offset();
};
} // namespace crib::internal::vase
