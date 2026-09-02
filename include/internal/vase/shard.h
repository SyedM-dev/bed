#pragma once

#include "constants.h"
#include "pch.h"
#include "storage/original.h"
#include "storage/storage.h"

namespace bed::internal::vase {
struct Shard {
  enum struct Kind : uint8_t {
    Branch,
    Petal
  } kind;

  uint16_t height;
  std::atomic_uint32_t refs;

  uint64_t length;
  uint64_t lines;

  Shard(Kind kind, uint64_t length, uint64_t lines, uint16_t height)
      : kind(kind), height(height), refs(1), length(length), lines(lines) {};

  static void retain(Shard *n);
  static void release(Shard *n);

  static Shard *from_file(const std::filesystem::path &path, bool posix_ending);
  static Shard *from_string(const char *data, uint64_t len, bool posix_ending);
  static Shard *from_command(const char *cmd, bool posix_ending);

  static std::pair<Shard *, Shard *> split(Shard *n, uint64_t offset);
  static Shard *concat(Shard *a, Shard *b);
  static Shard *merge_leaves(Shard *a, Shard *b);
  static Shard *append(Shard *root, Shard *leaf);
  static Shard *build(Shard **pieces, uint64_t lo, uint64_t hi);
};

struct Branch : Shard {
  Shard *left;
  Shard *right;

  Branch(Shard *l, Shard *r)
      : Shard(
          Kind::Branch,
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
  Storage *source;
  uint64_t pos;

  Petal(uint64_t length, uint64_t lines, Storage *source, uint64_t pos)
      : Shard(Kind::Petal, length, lines, 1),
        source(source), pos(pos) {
    source->retain();
  };
};
} // namespace bed::internal::vase
