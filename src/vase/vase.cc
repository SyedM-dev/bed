#include "vase/vase.h"

uint32_t Vase::offset_of(uint32_t line_number, uint32_t col) {
  if (line_number == 0)
    return col;

  Shard *s = root;
  uint32_t nth = line_number;
  uint32_t base_offset = 0;

  while (s->kind == Shard::ShardKind::Branch) {
    auto *b = static_cast<Branch *>(s);
    if (nth <= b->left->lines) {
      s = b->left;
    } else {
      nth -= b->left->lines;
      base_offset += b->left->length;
      s = b->right;
    }
  }

  auto *petal = static_cast<Petal *>(s);
  uint32_t nl_pos = petal->source->nth_newline(petal->pos, nth);
  uint32_t offset_in_petal = (nl_pos - petal->pos) + 1;
  return base_offset + offset_in_petal + col;
}
