#pragma once

#include "pch.h"

struct Buffer {
  virtual const char *read(uint64_t pos) = 0;
  virtual uint64_t length() = 0;
  virtual ~Buffer() = default;
};
