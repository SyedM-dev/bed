#include "vase/shard.h"

int height(Shard *n) {
  return n ? n->height : 0;
}

int balance_factor(Shard *n) {
  Branch *b = (Branch *)n;
  return height(b->left.ptr) - height(b->right.ptr);
}

ShardPtr rotate_right(Branch *z) {
  Branch *y = (Branch *)z->left.ptr;

  return new Branch(
    y->left.ptr,
    new Branch(y->right.ptr, z->right.ptr)
  );
}

ShardPtr rotate_left(Branch *z) {
  Branch *y = (Branch *)z->right.ptr;

  return new Branch(
    new Branch(z->left.ptr, y->left.ptr),
    y->right.ptr
  );
}

ShardPtr balance(Shard *node) {
  if (!node || node->kind == Shard::ShardKind::Petal)
    return ShardPtr(node);

  Branch *b = (Branch *)node;

  int bf = balance_factor(node);

  // left heavy
  if (bf > 1) {
    Branch *left = (Branch *)b->left.ptr;

    // Left-right case
    if (balance_factor(left) < 0) {
      auto new_left = rotate_left(left);

      auto rebuilt = new Branch(
        new_left.ptr,
        b->right.ptr
      );

      return rotate_right((Branch *)rebuilt);
    }

    // Left-left case
    return rotate_right(b);
  }

  // right heavy
  if (bf < -1) {
    Branch *right = (Branch *)b->right.ptr;

    // Right-left case
    if (balance_factor(right) > 0) {
      auto new_right = rotate_right(right);

      auto rebuilt = new Branch(
        b->left.ptr,
        new_right.ptr
      );

      return rotate_left((Branch *)rebuilt);
    }

    // Right-right case
    return rotate_left(b);
  }

  return ShardPtr(node);
}

ShardPtr merge(Shard *a, Shard *b) {
  if (!a)
    return ShardPtr(b);
  if (!b)
    return ShardPtr(a);

  if (a->height > b->height + 1) {
    Branch *ba = (Branch *)a;
    auto r = merge(ba->right.ptr, b);
    return balance(new Branch(ba->left.ptr, r.ptr));
  }

  if (b->height > a->height + 1) {
    Branch *bb = (Branch *)b;
    auto l = merge(a, bb->left.ptr);
    return balance(new Branch(l.ptr, bb->right.ptr));
  }

  return balance(new Branch(a, b));
}

std::pair<ShardPtr, ShardPtr> split_shard(Shard *n, uint32_t offset) {
  if (!n)
    return {nullptr, nullptr};
  if (offset == 0)
    return {nullptr, ShardPtr(n)};
  if (offset == n->length)
    return {ShardPtr(n), nullptr};

  if (n->kind == Shard::ShardKind::Branch) {
    Branch *b = (Branch *)n;
    if (offset < b->left.ptr->length) {
      auto [a, b2] = split_shard(b->left.ptr, offset);
      return {a, merge(b2.ptr, b->right.ptr)};
    } else {
      auto [a, b2] = split_shard(b->right.ptr, offset - b->left.ptr->length);
      return {merge(b->left.ptr, a.ptr), b2};
    }
  } else {
    Petal *p = (Petal *)n;
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

    return {ShardPtr(left), ShardPtr(right)};
  }
}

ShardPtr merge_leaves(Shard *a, Shard *b) {
  if (a->kind != Shard::ShardKind::Petal || b->kind != Shard::ShardKind::Petal)
    return merge(a, b);
  Petal *pa = (Petal *)a;
  Petal *pb = (Petal *)b;
  if (!(pa->source == pb->source && pa->pos + pa->length == pb->pos))
    return merge(a, b);
  return new Petal(
    pa->length + pb->length,
    pa->lines + pb->lines,
    pa->source,
    pa->pos
  );
}

ShardPtr append_leaf(Shard *root, Shard *leaf) {
  if (!root)
    return ShardPtr(leaf);
  if (root->kind == Shard::ShardKind::Petal)
    return merge_leaves(root, leaf);

  Branch *b = (Branch *)root;
  auto new_right = append_leaf(b->right.ptr, leaf);

  return balance(new Branch(b->left.ptr, new_right.ptr));
}

ShardPtr concat_shard(ShardPtr left, ShardPtr right) {
  return merge(left.ptr, right.ptr);
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
      << " height=" << (int)shard->height
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
