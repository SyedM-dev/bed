#pragma once

#include "buffer.h"
#include "pch.h"

struct OriginalBuffer : Buffer {
  char *buf;
  uint32_t length;
  std::vector<uint32_t> newlines;

  // TODO: replace with file io or even lazy loaded file io for large files/logs etc. later.
  OriginalBuffer(char *buf, uint32_t length);

  ~OriginalBuffer();

  const char *read(uint32_t pos, uint32_t *out_len);
  uint32_t count_lines(uint32_t pos, uint32_t length);
  uint32_t next_newline(uint32_t pos);
  uint32_t nth_newline(uint32_t pos, uint32_t n);
};
