#pragma once

#include "buffer.h"
#include "pch.h"

struct OriginalBuffer : Buffer {
  char *buf;
  uint32_t len;

  // Add 2 modes here, normal and lazy, lazy copies the file to a safe location and uses mmap.
  OriginalBuffer(std::string path);

  ~OriginalBuffer();

  const char *read(uint32_t pos, uint32_t *out_len);
  uint32_t length();
};
