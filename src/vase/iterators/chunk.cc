#include "vase/iterators/chunk.h"

ChunkIterator::ChunkIterator(Shard *r, uint32_t offset) : root(r) {
  if (!root)
    return;
  Shard::retain(root);
  seek(offset);
}

ChunkIterator::~ChunkIterator() {
  Shard::release(root);
}

void ChunkIterator::seek(uint32_t offset) {
  stack.clear();
  petal = nullptr;
  petal_offset = 0;
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
        curr = b->right;
      }
    }
  }
}

bool ChunkIterator::next(const char **data, uint32_t *out_len) {
  while (true) {
    if (petal && petal_offset < petal->length) {
      uint32_t remaining = petal->length - petal_offset;
      uint32_t read_pos = petal->pos + petal_offset;
      uint32_t got = 0;
      const char *chunk = petal->source->read(read_pos, &got);
      if (got == 0) {
        petal = nullptr;
        continue;
      }
      uint32_t take = std::min(got, remaining);
      *data = chunk;
      *out_len = take;
      petal_offset += take;
      return true;
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
