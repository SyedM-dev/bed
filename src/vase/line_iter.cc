#include "vase/line_iter.h"

LineIterator::LineIterator(Shard *r, uint32_t start_line) : root(r) {
  if (!root)
    return;
  Shard::retain(root);
  seek_line(start_line);
}

LineIterator::~LineIterator() {
  Shard::release(root);
}

void LineIterator::descend(Shard *s) {
  while (s->kind == Shard::ShardKind::Branch) {
    auto *b = static_cast<Branch *>(s);
    stack.push_back(b);
    s = b->left;
  }
  petal = static_cast<Petal *>(s);
  petal_offset = 0;
}

bool LineIterator::advance_petal() {
  while (!stack.empty()) {
    Branch *top = stack.back();
    stack.pop_back();
    descend(top->right);
    if (petal->length > 0)
      return true;
  }
  petal = nullptr;
  return false;
}

void LineIterator::seek_line(uint32_t line_index) {
  stack.clear();
  if (line_index == 0) {
    descend(root);
    return;
  }
  uint32_t nth = line_index; // we want the position right after the nth '\n' (1-indexed)
  Shard *s = root;
  while (s->kind == Shard::ShardKind::Branch) {
    auto *b = static_cast<Branch *>(s);
    if (nth <= b->left->lines) {
      stack.push_back(b); // still need to visit b->right later
      s = b->left;
    } else {
      nth -= b->left->lines; // left fully skipped, don't revisit it
      s = b->right;
    }
  }
  petal = static_cast<Petal *>(s);
  uint32_t nl_pos = petal->source->nth_newline(petal->pos, nth);
  petal_offset = (nl_pos - petal->pos) + 1;
  if (petal_offset >= petal->length)
    advance_petal();
}

bool LineIterator::next(std::string &line) {
  line.clear();
  if (!petal)
    return false;

  bool produced = false;
  while (petal) {
    produced = true;
    uint32_t abs_pos = petal->pos + petal_offset;
    uint32_t petal_end = petal->pos + petal->length;

    uint32_t nl = petal->source->next_newline(abs_pos);
    uint32_t stop = (nl != UINT32_MAX && nl < petal_end) ? nl : petal_end;

    uint32_t remaining = stop - abs_pos;
    uint32_t cur = abs_pos;
    while (remaining) {
      uint32_t got;
      const char *data = petal->source->read(cur, &got);
      uint32_t take = std::min(got, remaining);
      line.append(data, take);
      cur += take;
      remaining -= take;
    }
    petal_offset = cur - petal->pos;

    if (stop == nl) {
      petal_offset += 1;
      if (petal_offset >= petal->length)
        advance_petal();
      return true;
    }

    if (!advance_petal())
      break;
  }
  return produced;
}
