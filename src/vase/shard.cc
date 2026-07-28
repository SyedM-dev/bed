#include "vase/shard.h"

int height(Shard *n) {
  return n ? n->height : 0;
}

int balance_factor(Shard *n) {
  Branch *b = (Branch *)n;
  return height(b->left) - height(b->right);
}

Shard *rotate_right(Branch *z) {
  Branch *y = (Branch *)z->left;

  Shard *middle = new Branch(y->right, z->right);
  Shard *out = new Branch(y->left, middle);

  Shard::release(middle);
  Shard::release(z);

  return out;
}

Shard *rotate_left(Branch *z) {
  Branch *y = (Branch *)z->right;

  Shard *middle = new Branch(z->left, y->left);
  Shard *out = new Branch(middle, y->right);

  Shard::release(middle);
  Shard::release(z);

  return out;
}

Shard *balance(Shard *node) {
  if (!node || node->kind == Shard::ShardKind::Petal)
    return node;

  Branch *b = (Branch *)node;
  int bf = balance_factor(node);

  if (bf > 1) {
    Branch *left = (Branch *)b->left;
    if (balance_factor(left) < 0) {
      Shard::retain(left);
      auto new_left = rotate_left(left);
      auto rebuilt = new Branch(new_left, b->right);
      auto result = rotate_right((Branch *)rebuilt);
      Shard::release(new_left);
      Shard::release(b);
      return result;
    }
    return rotate_right(b);
  }

  if (bf < -1) {
    Branch *right = (Branch *)b->right;
    if (balance_factor(right) > 0) {
      Shard::retain(right);
      auto new_right = rotate_right(right);
      auto rebuilt = new Branch(b->left, new_right);
      auto result = rotate_left((Branch *)rebuilt);
      Shard::release(new_right);
      Shard::release(b);
      return result;
    }
    return rotate_left(b);
  }

  return node;
}

Shard *merge(Shard *a, Shard *b) {
  if (!a)
    return b ? (Shard::retain(b), b) : nullptr;
  if (!b)
    return (Shard::retain(a), a);

  if (a->height > b->height + 1) {
    Branch *ba = (Branch *)a;
    Shard *r = merge(ba->right, b);
    Shard *out = balance(new Branch(ba->left, r));
    Shard::release(r);
    return out;
  }

  if (b->height > a->height + 1) {
    Branch *bb = (Branch *)b;
    Shard *l = merge(a, bb->left);
    Shard *out = balance(new Branch(l, bb->right));
    Shard::release(l);
    return out;
  }

  return balance(new Branch(a, b));
}

std::pair<Shard *, Shard *> split_shard(Shard *n, uint32_t offset) {
  if (!n)
    return {nullptr, nullptr};
  if (offset == 0) {
    Shard::retain(n);
    return {nullptr, n};
  }
  if (offset == n->length) {
    Shard::retain(n);
    return {n, nullptr};
  }

  if (n->kind == Shard::ShardKind::Branch) {
    Branch *b = (Branch *)n;
    if (offset < b->left->length) {
      auto [a, b2] = split_shard(b->left, offset);
      Shard *right = merge(b2, b->right);
      Shard::release(b2);
      return {a, right};
    } else {
      auto [a, b2] = split_shard(b->right, offset - b->left->length);
      Shard *left = merge(b->left, a);
      Shard::release(a);
      return {left, b2};
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
    return {left, right};
  }
}

Shard *merge_leaves(Shard *a, Shard *b) {
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

Shard *append_leaf(Shard *root, Shard *leaf) {
  if (!root)
    return leaf;
  if (root->kind == Shard::ShardKind::Petal)
    return merge_leaves(root, leaf);
  Branch *b = (Branch *)root;
  auto new_right = append_leaf(b->right, leaf);
  auto out = balance(new Branch(b->left, new_right));
  Shard::release(new_right);
  return out;
}

Shard *concat_shard(Shard *left, Shard *right) {
  return merge(left, right);
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
    print_shard(branch->left, depth + 2);

    std::cout << indent << "└─ right:\n";
    print_shard(branch->right, depth + 2);
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
