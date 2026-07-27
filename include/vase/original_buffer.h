#pragma once

#include "pch.h"

struct OriginalBuffer {
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
};
