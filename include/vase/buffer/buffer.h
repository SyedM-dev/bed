#pragma once

#include "pch.h"

struct Buffer {
  virtual const char *read(uint64_t pos, uint64_t *out_len) = 0;
  virtual inline uint64_t length() = 0;
  virtual ~Buffer() = default;
};
