#include "vase/buffer/append.h"

AppendBuffer::AppendBuffer() {
  t_current = (TChunk *)malloc(sizeof(TChunk));
  buf.push_back(t_current);
}

AppendBuffer::~AppendBuffer() {
  for (auto p : buf)
    free(p);
}

uint64_t AppendBuffer::append(char c) {
  if (t_offset == APPEND_CHUNK_SIZE) {
    t_current = (TChunk *)malloc(sizeof(TChunk));
    buf.push_back(t_current);
    t_offset = 0;
  }
  (*t_current)[t_offset++] = c;
  return current_offset++;
}

uint64_t AppendBuffer::append(const char *text, uint64_t length) {
  uint64_t start = current_offset;
  while (length > 0) {
    if (t_offset == APPEND_CHUNK_SIZE) {
      t_current = (TChunk *)malloc(sizeof(TChunk));
      buf.push_back(t_current);
      t_offset = 0;
    }
    uint64_t copy = std::min(length, APPEND_CHUNK_SIZE - t_offset);
    memcpy((*t_current) + t_offset, text, copy);
    t_offset += copy;
    current_offset += copy;
    text += copy;
    length -= copy;
  }
  return start;
}

const char *AppendBuffer::read(uint64_t pos, uint64_t *out_len) {
  if (pos >= current_offset)
    return nullptr;
  uint64_t local_offset = pos % APPEND_CHUNK_SIZE;
  if (out_len) {
    uint64_t remaining = current_offset - pos;
    uint64_t until_chunk_end = APPEND_CHUNK_SIZE - local_offset;
    *out_len = std::min(remaining, until_chunk_end);
  }
  return &(*buf[pos / APPEND_CHUNK_SIZE])[local_offset];
}

inline uint64_t AppendBuffer::length() {
  return current_offset;
}
