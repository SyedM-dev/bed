#include "vase/buffer/original.h"

const char *OriginalBuffer::read(uint32_t pos, uint32_t *out_len) {
  if (pos >= length)
    return nullptr;
  *out_len = length - pos;
  return buf + pos;
}

uint32_t OriginalBuffer::count_lines(uint32_t pos, uint32_t length) {
  auto start = std::lower_bound(
    newlines.begin(),
    newlines.end(),
    pos
  );

  auto end = std::lower_bound(
    start,
    newlines.end(),
    pos + length
  );

  return (uint32_t)(end - start);
}
