#pragma once

#include "pch.h"
#include "storage.h"

namespace bed::internal::vase {
struct OriginalStorage : Storage {
  std::atomic_uint64_t refs{0};
  const char *buf = nullptr;
  uint64_t len = 0;
  int fd = -1;

  OriginalStorage(std::filesystem::path base_dir);
  ~OriginalStorage();

  void initialize();
  const char *read(uint64_t pos) override;
  uint64_t length() override;

  void retain() override {
    refs++;
  }

  void release() override {
    if (--refs > 0)
      return;
    delete this;
  }
};
} // namespace bed::internal::vase
