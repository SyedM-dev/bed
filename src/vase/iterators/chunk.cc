#include "vase/iterators/chunk.h"

ChunkIterator::ChunkIterator(Shard *r) : root(r) {
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
  global_offset = 0;
  if (!root || offset >= root->length)
    return;
  uint32_t target = offset;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)curr;
      petal_offset = target;
      return;
    } else {
      auto *b = (Branch *)curr;
      uint32_t left_len = b->left->length;
      if (target < left_len) {
        stack.push_back(b->right);
        curr = b->left;
      } else {
        target -= left_len;
        global_offset += b->left->length;
        curr = b->right;
      }
    }
  }
}

void ChunkIterator::seek_line(uint32_t line) {
  stack.clear();
  petal = nullptr;
  petal_offset = 0;
  global_offset = 0;
  if (!root || line > root->lines)
    return;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)curr;
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
      petal_offset = offset;
      return;
    } else {
      auto *b = (Branch *)curr;
      uint32_t left_lines = b->left->lines;
      if (line <= left_lines) {
        stack.push_back(b->right);
        curr = b->left;
      } else {
        line -= left_lines;
        global_offset += b->left->length;
        curr = b->right;
      }
    }
  }
}

uint32_t ChunkIterator::byte_offset() {
  return global_offset + petal_offset;
}

bool ChunkIterator::next(const char **data, uint32_t *out_len) {
  while (true) {
    if (petal) {
      if (petal_offset < petal->length) {
        uint32_t remaining = petal->length - petal_offset;
        uint32_t read_pos = petal->pos + petal_offset;
        uint32_t got = 0;
        const char *chunk = petal->source->read(read_pos, &got);
        if (got == 0) {
          petal_offset = petal->length;
          petal = nullptr;
          continue;
        }
        uint32_t take = std::min(got, remaining);
        *data = chunk;
        *out_len = take;
        petal_offset += take;
        return true;
      }
      global_offset += petal->length;
      petal = nullptr;
      petal_offset = 0;
    }
    if (stack.empty())
      return false;
    Shard *s = stack.back();
    stack.pop_back();
    if (!s)
      continue;
    if (s->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)s;
      petal_offset = 0;
    } else {
      auto *b = (Branch *)s;
      stack.push_back(b->right);
      stack.push_back(b->left);
    }
  }
}
