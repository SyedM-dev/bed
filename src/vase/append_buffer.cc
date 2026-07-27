#include "vase/append_buffer.h"

AppendBuffer::AppendBuffer() {
  new_text_chunk();
  new_line_chunk();
}

AppendBuffer::~AppendBuffer() {
  for (auto p : buf)
    free(p);
  for (auto p : newlines)
    free(p);
}

inline void AppendBuffer::key(char c) {
  if (t_offset == CHUNK_SIZE)
    new_text_chunk();
  (*t_current)[t_offset++] = c;
  if (c == '\n') {
    if (l_offset == CHUNK_SIZE)
      new_line_chunk();
    (*l_current)[l_offset++] = current_offset;
  }
  ++current_offset;
}

void AppendBuffer::append(const char *text, uint32_t length) {
  while (length) {
    uint32_t space = CHUNK_SIZE - t_offset;
    if (space == 0) {
      new_text_chunk();
      space = CHUNK_SIZE;
    }
    uint32_t copy = length < space ? length : space;
    _append(text, copy);
    text += copy;
    length -= copy;
  }
}

inline void AppendBuffer::new_text_chunk() {
  t_current = (TChunk *)malloc(sizeof(TChunk));
  buf.push_back(t_current);
  t_offset = 0;
}

inline void AppendBuffer::new_line_chunk() {
  l_current = (LChunk *)malloc(sizeof(LChunk));
  newlines.push_back(l_current);
  l_offset = 0;
}

inline void AppendBuffer::_append(const char *text, uint32_t length) {
  memcpy((*t_current) + t_offset, text, length);

  const char *p = text;
  const char *end = text + length;

  while (p < end) {
    p = (const char *)memchr(p, '\n', end - p);
    if (!p)
      break;
    if (l_offset == CHUNK_SIZE)
      new_line_chunk();
    (*l_current)[l_offset++] = current_offset + uint32_t(p - text);
    p++;
  }

  t_offset += length;
  current_offset += length;
}
