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

  static Shard *from_file(std::filesystem::path path, OriginalBuffer *b);
  static std::vector<Shard *> from_swap(std::filesystem::path path, OriginalBuffer *b);
  static Shard *new_empty(OriginalBuffer *b);

  static std::pair<Shard *, Shard *> split(Shard *n, uint64_t offset);
  static Shard *concat(Shard *left, Shard *right);
  static Shard *merge(Shard *a, Shard *b);
  static Shard *merge_leaves(Shard *a, Shard *b);
  static Shard *append_leaf(Shard *root, Shard *leaf);
  static Shard *build_balanced(Shard **pieces, uint64_t lo, uint64_t hi);
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
