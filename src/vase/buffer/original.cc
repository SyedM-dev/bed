#include "vase/buffer/original.h"

OriginalBuffer::OriginalBuffer(char *buf, uint32_t len) : buf(buf), len(len) {}

OriginalBuffer::~OriginalBuffer() {
  free(buf);
}

const char *OriginalBuffer::read(uint32_t pos, uint32_t *out_len) {
  if (pos >= len)
    return nullptr;
  if (out_len)
    *out_len = len - pos;
  return buf + pos;
}

uint32_t OriginalBuffer::length() {
  return len;
}
