#pragma once

#include "append_buffer.h"
#include "original_buffer.h"
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
      Petal::Kind::Original,
      0
    );
  }
};
