#pragma once

#include "pch.h"

namespace bed::internal {
enum struct Direction : uint8_t {
  Forward,
  Backward
};

inline static bool write_all(int fd, const void *data, size_t len) {
  const char *p = (const char *)data;
  while (len > 0) {
    ssize_t n = write(fd, p, len);
    if (n > 0) {
      p += n;
      len -= (size_t)n;
      continue;
    }
    if (n == -1 && errno == EINTR)
      continue;
    return false;
  }
  return true;
}
} // namespace bed::internal
