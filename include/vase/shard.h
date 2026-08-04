#pragma once

#include "buffer/buffer.h"
#include "buffer/original.h"
#include "constants.h"
#include "pch.h"
#include "utils/utils.h"

struct Shard {
  enum struct ShardKind : uint8_t {
    Branch,
    Petal
  } kind;

  uint8_t height;

  uint64_t length;
  uint64_t lines;

  std::atomic_uint64_t refs;

  Shard(ShardKind kind, uint64_t length, uint64_t lines, uint8_t height)
      : kind(kind), height(height), length(length), lines(lines), refs(1) {};

  static void retain(Shard *n);
  static void release(Shard *n);
};

struct Branch : Shard {
  Shard *left;
  Shard *right;

  Branch(Shard *l, Shard *r)
      : Shard(
          ShardKind::Branch,
          l->length + r->length,
          l->lines + r->lines,
          1 + std::max(l->height, r->height)
        ),
        left(l), right(r) {
    retain(left);
    retain(right);
  };
};

struct Petal : Shard {
  Buffer *source;

  uint64_t pos;

  Petal(uint64_t length, uint64_t lines, Buffer *source, uint64_t pos)
      : Shard(ShardKind::Petal, length, lines, 1),
        source(source), pos(pos) {};
};

Shard *create_file_shards(std::string &path, OriginalBuffer *b);
Shard *create_swap_shards(std::string &path, OriginalBuffer *b);

std::pair<Shard *, Shard *> split_shard(Shard *n, uint64_t offset);
Shard *concat_shard(Shard *left, Shard *right);
Shard *merge(Shard *a, Shard *b);
Shard *merge_leaves(Shard *a, Shard *b);
Shard *append_leaf(Shard *root, Shard *leaf);
Shard *build_balanced(Shard **pieces, uint64_t lo, uint64_t hi);
