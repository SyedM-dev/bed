#include "vase/buffer/append.h"

AppendBuffer::AppendBuffer() {
  new_text_chunk();
}

AppendBuffer::~AppendBuffer() {
  for (auto p : buf)
    free(p);
}

uint32_t AppendBuffer::key(char c) {
  if (t_offset == CHUNK_SIZE)
    new_text_chunk();
  (*t_current)[t_offset++] = c;
  return current_offset++;
}

uint32_t AppendBuffer::append(const char *text, uint32_t length) {
  uint32_t start = current_offset;
  while (length > 0) {
    if (t_offset == CHUNK_SIZE)
      new_text_chunk();
    uint32_t copy = std::min(length, CHUNK_SIZE - t_offset);
    memcpy((*t_current) + t_offset, text, copy);
    t_offset += copy;
    current_offset += copy;
    text += copy;
    length -= copy;
  }
  return start;
}

const char *AppendBuffer::read(uint32_t pos, uint32_t *out_len) {
  if (pos >= current_offset)
    return nullptr;
  uint32_t local_offset = pos % CHUNK_SIZE;
  if (out_len) {
    uint32_t remaining = current_offset - pos;
    uint32_t until_chunk_end = CHUNK_SIZE - local_offset;
    *out_len = std::min(remaining, until_chunk_end);
  }
  return &(*buf[pos / CHUNK_SIZE])[local_offset];
}

inline uint32_t AppendBuffer::length() {
  return current_offset;
}

inline void AppendBuffer::new_text_chunk() {
  t_current = (TChunk *)malloc(sizeof(TChunk));
  buf.push_back(t_current);
  t_offset = 0;
}
