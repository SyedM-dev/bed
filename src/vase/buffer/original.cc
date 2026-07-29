#include "vase/buffer/original.h"

OriginalBuffer::OriginalBuffer(char *buf, uint32_t length) : buf(buf), length(length) {
  const char *p = buf;
  const char *end = buf + length;
  while (p < end) {
    p = (const char *)memchr(p, '\n', end - p);
    if (!p)
      break;
    newlines.push_back(uint32_t(p - buf));
    p++;
  }
}

OriginalBuffer::~OriginalBuffer() {
  free(buf);
}

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

uint32_t OriginalBuffer::next_newline(uint32_t pos) {
  auto it = std::lower_bound(newlines.begin(), newlines.end(), pos);
  return it == newlines.end() ? UINT32_MAX : *it;
}

uint32_t OriginalBuffer::nth_newline(uint32_t pos, uint32_t n) {
  auto it = std::lower_bound(newlines.begin(), newlines.end(), pos);
  if (uint32_t(newlines.end() - it) < n)
    return UINT32_MAX;
  return *(it + (n - 1));
}
