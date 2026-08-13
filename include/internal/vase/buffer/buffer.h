#pragma once

#include "pch.h"

namespace bed::internal::vase {
struct Buffer {
  virtual const char *read(uint64_t pos) = 0;
  virtual uint64_t length() = 0;
  virtual ~Buffer() = default;
};
} // namespace bed::internal::vase
