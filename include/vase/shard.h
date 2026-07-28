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
      : kind(kind), length(length), lines(lines), height(height), refs(0) {};

  virtual ~Shard() = default;
};

struct ShardPtr {
  Shard *ptr;

  ShardPtr(const ShardPtr &other) : ptr(other.ptr) {
    if (ptr)
      ptr->refs++;
  }

  ShardPtr &operator=(const ShardPtr &other) {
    if (ptr == other.ptr)
      return *this;
    if (ptr && --ptr->refs == 0)
      delete ptr;
    ptr = other.ptr;
    if (ptr)
      ptr->refs++;
    return *this;
  }

  ShardPtr(Shard *p = nullptr) : ptr(p) {
    if (ptr)
      ptr->refs++;
  }

  ~ShardPtr() {
    if (ptr && --ptr->refs == 0)
      delete ptr;
  }
};

struct Branch : Shard {
  ShardPtr left;
  ShardPtr right;

  Branch(Shard *l, Shard *r)
      : Shard(
          ShardKind::Branch,
          l->length + r->length,
          l->lines + r->lines,
          1 + std::max(l->height, r->height)
        ),
        left(l), right(r) {};
};

struct Petal : Shard {
  Buffer *source;

  uint32_t pos;

  Petal(uint32_t length, uint32_t lines, Buffer *source, uint32_t pos)
      : Shard(ShardKind::Petal, length, lines, 1),
        source(source), pos(pos) {};
};

std::pair<ShardPtr, ShardPtr> split_shard(Shard *n, uint32_t offset);
ShardPtr concat_shard(ShardPtr left, ShardPtr right);
ShardPtr merge(Shard *a, Shard *b);
ShardPtr merge_leaves(Shard *a, Shard *b);
ShardPtr append_leaf(Shard *root, Shard *leaf);

void print_shard(const Shard *shard, int depth = 0);
