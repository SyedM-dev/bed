#include "internal/vase/vase.h"

namespace crib::internal::vase {
void Shard::retain(Shard *n) {
  if (n)
    n->refs++;
};

void Shard::release(Shard *n) {
  if (!n || --n->refs > 0)
    return;
  if (n->kind == Shard::Kind::Branch) {
    release(((Branch *)n)->left);
    release(((Branch *)n)->right);
    delete (Branch *)n;
  } else {
    delete (Petal *)n;
  }
}

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
  if (!node || node->kind == Shard::Kind::Petal)
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

Shard *Shard::merge(Shard *a, Shard *b) {
  if (!a)
    return (Shard::retain(b), b);
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

std::pair<Shard *, Shard *> Shard::split(Shard *n, uint64_t offset) {
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
  if (n->kind == Shard::Kind::Branch) {
    Branch *b = (Branch *)n;
    if (offset < b->left->length) {
      auto [a, b2] = split(b->left, offset);
      Shard *right = merge(b2, b->right);
      Shard::release(b2);
      return {a, right};
    } else {
      auto [a, b2] = split(b->right, offset - b->left->length);
      Shard *left = merge(b->left, a);
      Shard::release(a);
      return {left, b2};
    }
  } else {
    Petal *p = (Petal *)n;
    uint64_t count[2]{0};
    const char *c = p->source->read(p->pos);
    const char *start = c;
    const char *end = c + p->length;
    while (c < end) {
      const char *nl = (const char *)memchr(c, '\n', end - c);
      if (!nl)
        break;
      if ((uint64_t)(nl - start) < offset)
        count[0]++;
      else
        count[1]++;
      c = nl + 1;
    }
    auto left = new Petal(offset, count[0], p->source, p->pos);
    auto right = new Petal(p->length - offset, count[1], p->source, p->pos + offset);
    return {left, right};
  }
}

Shard *Shard::merge_leaves(Shard *a, Shard *b) {
  if (a->kind != Shard::Kind::Petal || b->kind != Shard::Kind::Petal)
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

Shard *Shard::append(Shard *root, Shard *leaf) {
  if (!root) {
    Shard::retain(leaf);
    return leaf;
  }
  if (root->kind == Shard::Kind::Petal && root->length < PETAL_SIZE_MAX)
    return merge_leaves(root, leaf);
  Branch *b = (Branch *)root;
  auto new_right = append(b->right, leaf);
  auto out = balance(new Branch(b->left, new_right));
  Shard::release(new_right);
  return out;
}

Shard *Shard::concat(Shard *left, Shard *right) {
  return merge(left, right);
}

Shard *Shard::build(Shard **pieces, uint64_t lo, uint64_t hi) {
  if (hi - lo == 1)
    return pieces[lo];
  size_t mid = lo + (hi - lo) / 2;
  Shard *left = build(pieces, lo, mid);
  Shard *right = build(pieces, mid, hi);
  Shard *node = new Branch(left, right);
  Shard::release(left);
  Shard::release(right);
  return node;
}

Shard *Shard::from_file(std::filesystem::path path, OriginalBuffer *o, bool posix_ending) {
  int dest_fd = o->fd;
  if (dest_fd == -1)
    return nullptr;
  int src_fd = open(path.c_str(), O_RDONLY);
  if (src_fd == -1)
    return nullptr;

  uint64_t total = std::filesystem::file_size(path);
  if (posix_ending && total > 0) {
    char last;
    char s_last;
    if (pread(src_fd, &last, 1, (off_t)(total - 1)) != 1)
      return nullptr;
    if (pread(src_fd, &s_last, 1, (off_t)(total - 2)) != 1)
      return nullptr;
    if (last == '\n') {
      total--;
      if (s_last == '\r')
        total--;
    }
  }
  std::vector<Shard *> pieces;
  uint64_t pos = 0;
  pieces.reserve((total + PETAL_SIZE_MAX - 1) / PETAL_SIZE_MAX);
  char buf[PETAL_SIZE_MAX];

  while (pos < total) {
    uint64_t want = std::min(PETAL_SIZE_MAX, total - pos);
    ssize_t got = pread(src_fd, buf, want, pos);
    if (got <= 0) {
      close(src_fd);
      return nullptr;
    }
    uint64_t take = (uint64_t)got;
    uint64_t lines = 0;
    const char *p = buf;
    const char *end = p + take;
    while (p < end) {
      const void *nl = memchr(p, '\n', end - p);
      if (!nl)
        break;
      lines++;
      p = (const char *)nl + 1;
    }
    ssize_t written = write(dest_fd, buf, take);
    if (written != (ssize_t)take) {
      close(src_fd);
      return nullptr;
    }
    pieces.push_back(new Petal(take, lines, o, pos));
    pos += take;
  }

  close(src_fd);

  if (pieces.empty())
    return nullptr;

  o->initialize();

  if (pieces.size() == 1)
    return pieces[0];
  return build(pieces.data(), 0, pieces.size());
}

void Shard::dump(Shard *node, int depth) {
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
    auto *branch = (Branch *)node;
    std::cout << indent << "  left:\n";
    dump(branch->left, depth + 2);
    std::cout << indent << "  right:\n";
    dump(branch->right, depth + 2);
  } else {
    auto *petal = static_cast<Petal *>(node);
    constexpr auto clean = [](const std::string &text) {
      std::string result = text;
      size_t pos = 0;
      while ((pos = result.find('\n', pos)) != std::string::npos) {
        result.replace(pos, 1, "\\n");
        pos += 2;
      }
      return result;
    };
    std::cout
      << indent << "  source=" << petal->source
      << " pos=" << petal->pos
      << " length=" << petal->length
      << " lines=" << petal->lines
      << " text=\"" << clean(std::string(petal->source->read(petal->pos), petal->length))
      << "\"\n";
  }
}
} // namespace crib::internal::vase
