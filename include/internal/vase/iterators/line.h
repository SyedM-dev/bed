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

struct Iterator {
  Shard *root;
  std::string line;

  Iterator(Shard *root, uint64_t line_num)
      : root(root), it(root, line_num, Direction::Forward) {
    Shard::retain(root);
  }

  ~Iterator() {
    Shard::release(root);
  }

  Iterator(const Iterator &other)
      : root(other.root), it(other.it) {
    Shard::retain(root);
  }

  Iterator &operator=(const Iterator &other) {
    if (this != &other) {
      Shard::retain(other.root);
      Shard::release(root);
      root = other.root;
      it = other.it;
    }
    return *this;
  }

  Iterator(Iterator &&other) noexcept
      : root(other.root), it(std::move(other.it)) {
    other.root = nullptr;
  }

  Iterator &operator=(Iterator &&other) noexcept {
    if (this != &other) {
      Shard::release(root);
      root = other.root;
      it = std::move(other.it);
      other.root = nullptr;
    }
    return *this;
  }

  std::string &next() {
    it.next(&line);
    return line;
  }

private:
  LineIterator it;
};
} // namespace crib::internal::vase
