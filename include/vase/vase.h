#pragma once

#include "buffer/append.h"
#include "buffer/original.h"
#include "pch.h"
#include "shard.h"

struct Vase {
  OriginalBuffer original;
  AppendBuffer append;

  std::vector<ShardPtr> undo;
  uint8_t top; // of the undo stack.
  uint8_t max; // for redo when no edits have been done after some undo.

  ShardPtr root;

  Vase(char *data, uint32_t length)
      : original(data, length),
        append() {
    root = new Petal(
      length,
      original.newlines.size(),
      &original,
      0
    );
  }

  uint32_t length() {
    return root.ptr->length;
  }

  void insert(uint32_t offset, const char *data, uint32_t len) {
    uint32_t lines = 0;
    uint32_t pos = append.append(data, len, &lines);

    ShardPtr inserted = new Petal(len, lines, &append, pos);

    auto [left, right] = split_shard(root.ptr, offset);

    left = append_leaf(left.ptr, inserted.ptr);

    root = concat_shard(left, right);
  }
};
