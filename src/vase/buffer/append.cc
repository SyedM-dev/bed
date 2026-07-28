#include "vase/buffer/append.h"
#include <cstdint>

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

uint32_t AppendBuffer::find_newline(uint32_t target, uint32_t low, uint32_t high) {
  if (low >= high)
    return low;

  uint32_t first_chunk = low / CHUNK_SIZE;
  uint32_t last_chunk = (high - 1) / CHUNK_SIZE;

  uint32_t left = first_chunk;
  uint32_t right = last_chunk;

  while (left < right) {
    uint32_t mid = left + (right - left) / 2;

    uint32_t first_value = (*newlines[mid])[0];

    if (first_value < target)
      left = mid + 1;
    else
      right = mid;
  }

  uint32_t chunk = left;

  if (chunk > first_chunk) {
    LChunk &prev = *newlines[chunk - 1];

    uint32_t prev_size =
      (chunk - 1 == newlines.size() - 1) ? l_offset : CHUNK_SIZE;

    if (prev[prev_size - 1] >= target)
      chunk--;
  }

  LChunk &c = *newlines[chunk];

  uint32_t chunk_size =
    (chunk == newlines.size() - 1) ? l_offset : CHUNK_SIZE;

  uint32_t l = 0;
  uint32_t r = chunk_size;

  while (l < r) {
    uint32_t m = l + (r - l) / 2;

    if (c[m] < target)
      l = m + 1;
    else
      r = m;
  }

  return chunk * CHUNK_SIZE + l;
}

uint32_t AppendBuffer::count_lines(uint32_t pos, uint32_t length) {
  if (newlines.empty())
    return 0;

  uint32_t total_newlines = CHUNK_SIZE * (newlines.size() - 1) + l_offset;
  uint32_t pos_index = find_newline(pos, 0, total_newlines);
  uint32_t end_index = find_newline(pos + length, pos_index, total_newlines);

  return end_index - pos_index;
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
