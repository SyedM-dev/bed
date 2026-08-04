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

void ChunkIterator::seek_offset(uint64_t offset) {
  stack.clear();
  petal = nullptr;
  petal_offset = 0;
  global_offset = 0;
  global_line = 0;
  at_end = false;
  if (!root)
    return;
  if (offset >= root->length)
    offset = root->length - 1;
  uint64_t target = offset;
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)curr;
      petal_offset = target;
      global_offset += target;
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

void ChunkIterator::seek_line(uint64_t line) {
  stack.clear();
  petal = nullptr;
  petal_offset = 0;
  global_offset = 0;
  at_end = false;
  bool last_line = false;
  if (!root)
    return;
  if (dir == Direction::Backward) {
    if (line > root->lines + 1)
      line = root->lines + 1;
    if (line == root->lines + 1)
      last_line = true;
  } else {
    if (line > root->lines)
      line = root->lines;
    if (line == root->lines)
      last_line = true;
  }
  Shard *curr = root;
  while (curr) {
    if (curr->kind == Shard::ShardKind::Petal) {
      petal = (Petal *)curr;
      if (last_line && dir == Direction::Backward) {
        petal_offset = petal->length;
        global_offset += petal->length;
      } else {
        const char *text = nullptr;
        uint64_t remaining = 0;
        uint64_t offset = 0;
        while (line) {
          if (!text) {
            uint64_t got = 0;
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
        if (last_line && dir == Direction::Forward)
          at_end = true;
        petal_offset = offset;
        global_offset += offset;
      }
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

uint64_t ChunkIterator::byte_offset() {
  return global_offset;
}

uint64_t ChunkIterator::line_offset() {
  return global_line;
}

bool ChunkIterator::next(const char **data, uint64_t *out_len) {
  if (dir == Direction::Forward) {
    while (true) {
      if (petal) {
        if (petal_offset == petal->length) {
          if (at_end) {
            *data = nullptr;
            *out_len = 0;
            at_end = false;
            return true;
          }
          petal = nullptr;
          continue;
        }
        uint64_t remaining = petal->length - petal_offset;
        uint64_t got = 0;
        const char *chunk = petal->source->read(petal->pos + petal_offset, &got);
        if (got == 0) {
          petal = nullptr;
          petal_offset = 0;
          return false;
        }
        uint64_t take = std::min(got, remaining);
        *data = chunk;
        *out_len = take;
        petal_offset += take;
        global_offset += take;
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
        uint64_t got = 0;
        *data = petal->source->read(petal->pos, &got);
        if (petal_offset > got) {
          uint64_t got2 = 0;
          *data = petal->source->read(petal->pos + got, &got2);
          *out_len = std::min(got2, petal_offset - got);
          petal_offset = got;
          global_offset -= *out_len;
          return true;
        } else {
          *out_len = std::min(got, petal_offset);
          petal_offset = 0;
          global_offset -= *out_len;
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
