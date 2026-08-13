#pragma once

#include "buffer/buffer.h"
#include "buffer/original.h"
#include "constants.h"
#include "pch.h"

namespace bed::internal::vase {
struct Shard {
  enum struct Kind : uint8_t {
    Branch,
    Petal
  } kind;

  uint8_t height;

  uint64_t length;
  uint64_t lines;

  std::atomic_uint64_t refs;

  Shard(Kind kind, uint64_t length, uint64_t lines, uint8_t height)
      : kind(kind), height(height), length(length), lines(lines), refs(1) {};

  static void retain(Shard *n);
  static void release(Shard *n);

  static Shard *from_file(std::filesystem::path &path, OriginalBuffer *b, bool posix_ending);
  static Shard *from_command(const char *cmd, OriginalBuffer *o, bool posix_ending);
  static std::vector<Shard *> from_swap(std::filesystem::path &path, OriginalBuffer *b);

  static std::pair<Shard *, Shard *> split(Shard *n, uint64_t offset);
  static Shard *concat(Shard *a, Shard *b);
  static Shard *merge_leaves(Shard *a, Shard *b);
  static Shard *append(Shard *root, Shard *leaf);
  static Shard *build(Shard **pieces, uint64_t lo, uint64_t hi);
  static void dump(Shard *node, int depth = 0);
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
  Buffer *source;
  uint64_t pos;

  Petal(uint64_t length, uint64_t lines, Buffer *source, uint64_t pos)
      : Shard(Kind::Petal, length, lines, 1),
        source(source), pos(pos) {};
};

} // namespace bed::internal::vase
