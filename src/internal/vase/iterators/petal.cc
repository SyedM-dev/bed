#include "internal/vase/vase.h"

namespace crib::internal::vase {
PetalIterator::PetalIterator(Shard *r, Direction dir)
    : dir(dir), root(r) {
  if (!root)
    return;
}

void PetalIterator::seek_offset(uint64_t offset) {
  stack.clear();
  petal = nullptr;
  global_offset = 0;
  last_offset = 0;
  global_line = 0;
  if (!root)
    return;
  if (offset >= root->length)
    offset = root->length - 1;
  uint64_t target = offset;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::Kind::Petal) {
      petal = (Petal *)curr;
      petal_offset = target;
      global_offset += target;
      last_offset = global_offset;
      return;
    } else {
      auto *b = (Branch *)curr;
      uint64_t left_len = b->left->length;
      if (target < left_len) {
        if (dir == Direction::Forward)
          stack.push_back(b->right);
        curr = b->left;
      } else {
        target -= left_len;
        if (dir == Direction::Backward)
          stack.push_back(b->left);
        global_offset += left_len;
        global_line += b->left->lines;
        curr = b->right;
      }
    }
  }
  std::unreachable();
}

void PetalIterator::seek_line(uint64_t line) {
  stack.clear();
  petal = nullptr;
  global_offset = 0;
  last_offset = 0;
  bool last_line = false;
  if (!root)
    return;
  if (line > root->lines)
    line = root->lines;
  if (line == root->lines)
    last_line = true;
  if (!last_line && dir == Direction::Backward)
    line++;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::Kind::Petal) {
      petal = (Petal *)curr;
      if (last_line && dir == Direction::Backward) {
        petal_offset = petal->length;
        global_offset += petal->length;
      } else {
        const char *text = petal->source->read(petal->pos);
        uint64_t offset = 0;
        while (line) {
          const char *c = (const char *)memchr(text, '\n', petal->length - offset);
          if (!c)
            throw std::runtime_error("leaf line count is wrong.");
          offset += (c - text) + 1;
          text = c + 1;
          line--;
        }
        if (dir == Direction::Backward)
          --offset;
        petal_offset = offset;
        global_offset += offset;
      }
      last_offset = global_offset;
      return;
    } else {
      auto *b = (Branch *)curr;
      uint64_t left_lines = b->left->lines;
      if (line <= left_lines) {
        if (dir == Direction::Forward)
          stack.push_back(b->right);
        curr = b->left;
      } else {
        line -= left_lines;
        if (dir == Direction::Backward)
          stack.push_back(b->left);
        global_offset += b->left->length;
        curr = b->right;
      }
    }
  }
  std::unreachable();
}

Petal *PetalIterator::_next(uint64_t *offset) {
  while (true) {
    if (petal) {
      if (dir == Direction::Forward) {
        if (petal_offset == petal->length) {
          petal = nullptr;
          continue;
        }
      } else {
        if (petal_offset == 0) {
          petal = nullptr;
          continue;
        }
      }
      auto ret = petal;
      last_offset = global_offset;
      if (dir == Direction::Forward)
        global_offset += petal_offset;
      else
        global_offset -= petal_offset;
      petal = nullptr;
      *offset = petal_offset;
      return ret;
    }
    if (stack.empty())
      return nullptr;
    auto s = stack.back();
    stack.pop_back();
    while (s->kind == Shard::Kind::Branch) {
      Branch *b = (Branch *)s;
      if (dir == Direction::Forward) {
        stack.push_back(b->right);
        s = b->left;
      } else {
        stack.push_back(b->left);
        s = b->right;
      }
    }
    petal = (Petal *)s;
    petal_offset = dir == Direction::Forward ? 0 : petal->length;
  }
}

bool PetalIterator::next(const char **data, uint64_t *out_len) {
  uint64_t offset = 0;
  Petal *p = _next(&offset);
  if (!p) {
    *data = nullptr;
    *out_len = 0;
    return false;
  }
  if (dir == Direction::Forward) {
    *out_len = p->length - offset;
    *data = p->source->read(p->pos + offset);
  } else {
    *out_len = offset;
    *data = p->source->read(p->pos);
  }
  return true;
}

uint64_t PetalIterator::byte_offset() {
  return last_offset;
}
} // namespace crib::internal::vase
