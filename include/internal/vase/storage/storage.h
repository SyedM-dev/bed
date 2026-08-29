#pragma once

#include "pch.h"

namespace bed::internal::vase {
struct Storage {
  virtual const char *read(uint64_t pos) = 0;
  virtual uint64_t length() = 0;
  virtual ~Storage() = default;
  virtual void retain() = 0;
  virtual void release() = 0;
};
} // namespace bed::internal::vase
