#pragma once

#include "pch.h"

struct Buffer {
  virtual const char *read(uint32_t pos, uint32_t *out_len) = 0;
  virtual uint32_t count_lines(uint32_t pos, uint32_t length) = 0;
  virtual uint32_t next_newline(uint32_t pos) = 0;
  virtual uint32_t nth_newline(uint32_t pos, uint32_t n) = 0;
  virtual ~Buffer() = default;
};
