#pragma once

#include "../constants.h"
#include "pch.h"
#include "storage.h"

namespace bed::internal::vase {
struct AppendStorage : Storage {
  char *buf = nullptr;
  uint64_t allocated_capacity = 0;
  uint64_t current_size = 0;
  int fd = -1;

  AppendStorage(std::filesystem::path base_dir);
  ~AppendStorage();

  uint64_t append(const char c);
  uint64_t append(const char *text, uint64_t len);
  const char *read(uint64_t pos) override;
  uint64_t length() override;

  void retain() override {}
  void release() override {}

private:
  void grow(uint64_t len);
};
} // namespace bed::internal::vase
