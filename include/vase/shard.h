#pragma once

#include "pch.h"

struct Shard {
  enum struct ShardKind {
    Branch,
    Petal
  } kind;

  uint32_t length;
  uint32_t lines;

  std::atomic_uint32_t refs;

  Shard(ShardKind kind, uint32_t length, uint32_t lines)
      : kind(kind), length(length), lines(lines) {};
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
      : Shard(ShardKind::Branch, l->length + r->length, l->lines + r->lines),
        left(ShardPtr(l)), right(ShardPtr(r)) {};
};

struct Petal : Shard {
  enum struct Kind {
    Original,
    Append
  } source;

  uint32_t pos;

  Petal(uint32_t length, uint32_t lines, Kind source, uint32_t pos)
      : Shard(ShardKind::Petal, length, lines), source(source), pos(pos) {};
};
