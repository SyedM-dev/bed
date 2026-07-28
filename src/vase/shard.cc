#include "vase/shard.h"

std::pair<ShardPtr, ShardPtr> split(Shard *n, uint32_t offset) {
  if (!n)
    return {nullptr, nullptr};

  if (offset == 0)
    return {nullptr, ShardPtr(n)};

  if (offset == n->length)
    return {ShardPtr(n), nullptr};

  if (n->kind == Shard::ShardKind::Branch) {
    Branch *b = static_cast<Branch *>(n);

    if (offset < b->left.ptr->length) {
      auto [a, b2] = split(b->left.ptr, offset);

      return {
        a,
        ShardPtr(new Branch(
          b2.ptr,
          b->right.ptr
        ))
      };
    } else {
      auto [a, b2] = split(
        b->right.ptr,
        offset - b->left.ptr->length
      );

      return {
        ShardPtr(new Branch(
          b->left.ptr,
          a.ptr
        )),
        b2
      };
    }
  }

  Petal *p = static_cast<Petal *>(n);

  auto left = new Petal(
    offset,
    p->source->count_lines(0, offset),
    p->source,
    p->pos
  );

  auto right = new Petal(
    p->length - offset,
    p->source->count_lines(offset, p->length),
    p->source,
    p->pos + offset
  );

  return {
    ShardPtr(left),
    ShardPtr(right)
  };
}

void print_shard(const Shard *shard, int depth) {
  if (!shard) {
    std::cout << std::string(depth * 2, ' ') << "<null>\n";
    return;
  }

  std::string indent(depth * 2, ' ');

  std::cout << indent;

  if (shard->kind == Shard::ShardKind::Branch) {
    auto *branch = static_cast<const Branch *>(shard);

    std::cout
      << "Branch"
      << " @" << shard
      << " len=" << shard->length
      << " lines=" << shard->lines
      << " refs=" << shard->refs.load()
      << "\n";

    std::cout << indent << "├─ left:\n";
    print_shard(branch->left.ptr, depth + 2);

    std::cout << indent << "└─ right:\n";
    print_shard(branch->right.ptr, depth + 2);
  } else {
    auto *petal = static_cast<const Petal *>(shard);

    std::cout
      << "Petal"
      << " @" << shard
      << " len=" << shard->length
      << " lines=" << shard->lines
      << " refs=" << shard->refs.load()
      << " source=@" << petal->source
      << " pos=" << petal->pos
      << "\n";
  }
}
