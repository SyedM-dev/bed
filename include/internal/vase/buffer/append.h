#pragma once

#include "../constants.h"
#include "buffer.h"
#include "pch.h"

struct AppendBuffer : Buffer {
  char *buf = nullptr;
  uint64_t allocated_capacity = 0;
  uint64_t current_size = 0;
  int fd = -1;

  AppendBuffer(std::filesystem::path base_dir);
  ~AppendBuffer();

  uint64_t append(const char c);
  uint64_t append(const char *text, uint64_t len);
  const char *read(uint64_t pos) override;
  uint64_t length() override;

private:
  void grow(uint64_t len);
};
