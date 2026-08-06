#pragma once

#include "buffer/buffer.h"
#include "buffer/original.h"
#include "constants.h"
#include "pch.h"
#include "utils/utils.h"

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

  static Shard *from_file(std::filesystem::path path, OriginalBuffer *b);
  static std::vector<Shard *> from_swap(std::filesystem::path path, OriginalBuffer *b);

  static std::pair<Shard *, Shard *> split(Shard *n, uint64_t offset);
  static Shard *concat(Shard *left, Shard *right);
  static Shard *merge(Shard *a, Shard *b);
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
  Buffer *source;
  uint64_t pos;

  Petal(uint64_t length, uint64_t lines, Buffer *source, uint64_t pos)
      : Shard(Kind::Petal, length, lines, 1),
        source(source), pos(pos) {};
};

extern inline void dump_shard(Shard *node, int depth = 0) {
  if (!node) {
    std::cout << std::string(depth * 2, ' ') << "<null>\n";
    return;
  }

  std::string indent(depth * 2, ' ');

  std::cout << indent
            << "Shard@" << node
            << " kind=";

  switch (node->kind) {
  case Shard::Kind::Branch:
    std::cout << "Branch";
    break;
  case Shard::Kind::Petal:
    std::cout << "Petal";
    break;
  }

  std::cout
    << " height=" << unsigned(node->height)
    << " length=" << node->length
    << " lines=" << node->lines
    << " refs=" << node->refs.load()
    << "\n";

  if (node->kind == Shard::Kind::Branch) {
    auto *branch = static_cast<Branch *>(node);

    std::cout << indent << "  left:\n";
    dump_shard(branch->left, depth + 2);

    std::cout << indent << "  right:\n";
    dump_shard(branch->right, depth + 2);
  } else {
    auto *petal = static_cast<Petal *>(node);

    std::cout
      << indent << "  source=" << petal->source
      << " pos=" << petal->pos
      << " length=" << petal->length
      << " lines=" << petal->lines
      << "\n";
  }

  if (!depth)
    std::cout << "\n\n";
}
