#pragma once

#include "../shard.h"
#include "iter.h"
#include "pch.h"

namespace crib::internal::vase {
struct PetalIterator {
  Direction dir;
  Shard *root = nullptr;
  std::vector<Shard *> stack;
  Petal *petal = nullptr;
  uint64_t global_offset = 0;
  uint64_t last_offset = 0;
  uint64_t petal_offset = 0;
  uint64_t global_line = 0;

  PetalIterator(Shard *r, Direction dir);
  ~PetalIterator() = default;

  void seek_offset(uint64_t offset);
  void seek_line(uint64_t offset);
  bool next(const char **data, uint64_t *out_len);
  uint64_t byte_offset();

private:
  Petal *_next(uint64_t *offset);
};
} // namespace crib::internal::vase
