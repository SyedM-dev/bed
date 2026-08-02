#pragma once

#include "buffer.h"
#include "pch.h"

struct OriginalBuffer : Buffer {
  char *buf;
  uint32_t len;

  // TODO: replace with file io or even lazy loaded file io for large files/logs etc. later.
  OriginalBuffer(char *buf, uint32_t len);

  ~OriginalBuffer();

  const char *read(uint32_t pos, uint32_t *out_len);
  uint32_t length();
};
