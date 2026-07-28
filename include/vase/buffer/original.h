#pragma once

#include "buffer.h"
#include "pch.h"

struct OriginalBuffer : Buffer {
  char *buf;
  uint32_t length;
  std::vector<uint32_t> newlines;

  // TODO: replace with file io or even lazy loaded file io for large files/logs etc. later.
  OriginalBuffer(char *buf, uint32_t length) : buf(buf), length(length) {
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

  ~OriginalBuffer() {
    free(buf);
  }

  const char *read(uint32_t pos, uint32_t *out_len);
  uint32_t count_lines(uint32_t pos, uint32_t length);
};
