#pragma once

#include "buffer.h"
#include "pch.h"

struct OriginalBuffer : Buffer {
  const char *buf;
  uint64_t len;
  int fd = -1;

  OriginalBuffer(std::filesystem::path base_dir);
  ~OriginalBuffer();

  void initialize();
  const char *read(uint64_t pos, uint64_t *out_len);
  uint64_t length();
};
