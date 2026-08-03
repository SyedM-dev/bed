#include "vase/buffer/original.h"
#include "io/file.h"

OriginalBuffer::OriginalBuffer(std::string path) {
  read_file(path.c_str(), &buf, &len);
}

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
