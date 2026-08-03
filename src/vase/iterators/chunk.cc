#include "vase/iterators/chunk.h"

ChunkIterator::ChunkIterator(Shard *r, Direction dir)
    : dir(dir), root(r) {
  if (!root)
    return;
  Shard::retain(root);
}

ChunkIterator::~ChunkIterator() {
  Shard::release(root);
}

void ChunkIterator::seek_offset(uint32_t offset) {
  stack.clear();
  petal = nullptr;
  petal_offset = 0;
  if (!root)
    return;
  if (offset >= root->length)
    offset = root->length - 1;
  uint32_t target = offset;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)curr;
      petal_offset = target;
      global_offset += target;
      return;
    } else {
      auto *b = (Branch *)curr;
      uint32_t left_len = b->left->length;
      if (target < left_len) {
        if (dir == Direction::Forward)
          stack.push_back(b->right);
        curr = b->left;
      } else {
        target -= left_len;
        if (dir == Direction::Backward)
          stack.push_back(b->left);
        global_offset += left_len;
        curr = b->right;
      }
    }
  }
  std::unreachable();
}

void ChunkIterator::seek_line(uint32_t line) {
  stack.clear();
  petal = nullptr;
  petal_offset = 0;
  bool last_line = false;
  if (!root)
    return;
  if (line > root->lines + 1)
    line = root->lines + 1;
  if (line == root->lines + 1)
    last_line = true;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)curr;
      if (last_line) {
        petal_offset = petal->length;
        global_offset += petal->length;
      } else {
        const char *text = nullptr;
        uint32_t remaining = 0;
        uint32_t offset = 0;
        while (line) {
          if (!text) {
            uint32_t got = 0;
            text = petal->source->read(petal->pos + offset, &got);
            remaining = std::min(got, petal->length - offset);
          }
          const char *c = (const char *)memchr(text, '\n', remaining);
          if (!c) {
            offset += remaining;
            text = nullptr;
            continue;
          }
          remaining -= (c - text) + 1;
          offset += (c - text) + 1;
          text = c + 1;
          line--;
        }
        if (dir == Direction::Backward)
          --offset;
        petal_offset = offset;
        global_offset += offset;
      }
      return;
    } else {
      auto *b = (Branch *)curr;
      uint32_t left_lines = b->left->lines;
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

uint32_t ChunkIterator::byte_offset() {
  return global_offset;
}

bool ChunkIterator::next(const char **data, uint32_t *out_len) {
  if (dir == Direction::Forward) {
    while (true) {
      if (petal) {
        if (petal_offset == petal->length) {
          petal = nullptr;
          continue;
        }
        uint32_t remaining = petal->length - petal_offset;
        uint32_t got = 0;
        const char *chunk = petal->source->read(petal->pos + petal_offset, &got);
        if (got == 0) {
          petal = nullptr;
          petal_offset = 0;
          return false;
        }
        uint32_t take = std::min(got, remaining);
        *data = chunk;
        *out_len = take;
        petal_offset += take;
        return true;
      }
      if (stack.empty())
        return false;
      auto s = stack.back();
      stack.pop_back();
      while (s->kind == Shard::ShardKind::Branch) {
        Branch *b = (Branch *)s;
        stack.push_back(b->right);
        s = b->left;
      }
      petal = (Petal *)s;
      petal_offset = 0;
    }
  } else {
    while (true) {
      if (petal) {
        if (petal_offset == 0) {
          petal = nullptr;
          continue;
        }
        uint32_t got = 0;
        *data = petal->source->read(petal->pos, &got);
        if (petal_offset > got) {
          uint32_t got2 = 0;
          *data = petal->source->read(petal->pos + got, &got2);
          *out_len = std::min(got2, petal_offset - got);
          petal_offset = got;
          return true;
        } else {
          *out_len = std::min(got, petal_offset);
          petal_offset = 0;
          return true;
        }
      }
      if (stack.empty())
        return false;
      auto s = stack.back();
      stack.pop_back();
      while (s->kind == Shard::ShardKind::Branch) {
        Branch *b = (Branch *)s;
        stack.push_back(b->left);
        s = b->right;
      }
      petal = (Petal *)s;
      petal_offset = petal->length;
    }
  }
}
