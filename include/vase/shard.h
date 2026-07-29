#pragma once

#include "buffer/buffer.h"
#include "pch.h"

struct Shard {
  enum struct ShardKind {
    Branch,
    Petal
  } kind;

  uint32_t length;
  uint32_t lines;

  uint8_t height;

  std::atomic_uint32_t refs;

  Shard(ShardKind kind, uint32_t length, uint32_t lines, uint8_t height)
      : kind(kind), length(length), lines(lines), height(height), refs(1) {};

  virtual ~Shard() = default;

  static void retain(Shard *n) {
    n->refs++;
  }

  static void release(Shard *n) {
    if (!n || --n->refs > 0)
      return;
    delete n;
  }
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

  ~Branch() {
    release(left);
    release(right);
  }
};

struct Petal : Shard {
  Buffer *source;

  uint32_t pos;

  Petal(uint32_t length, uint32_t lines, Buffer *source, uint32_t pos)
      : Shard(ShardKind::Petal, length, lines, 1),
        source(source), pos(pos) {};
};

std::pair<Shard *, Shard *> split_shard(Shard *n, uint32_t offset);
Shard *concat_shard(Shard *left, Shard *right);
Shard *merge(Shard *a, Shard *b);
Shard *merge_leaves(Shard *a, Shard *b);
Shard *append_leaf(Shard *root, Shard *leaf);

uint32_t offset_of(Shard *s, uint32_t line_number, uint32_t col);

void print_shard(const Shard *shard, int depth = 0);
