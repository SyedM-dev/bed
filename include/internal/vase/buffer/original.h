#pragma once

#include "buffer.h"
#include "pch.h"

namespace crib::internal::vase {
struct OriginalBuffer : Buffer {
  const char *buf;
  uint64_t len;
  int fd = -1;

  OriginalBuffer(std::filesystem::path base_dir);
  ~OriginalBuffer();

  void initialize();
  const char *read(uint64_t pos) override;
  uint64_t length() override;
};
} // namespace crib::internal::vase
