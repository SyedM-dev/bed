#pragma once

#include "../constants.h"
#include "buffer.h"
#include "pch.h"

struct AppendBuffer : Buffer {
  using TChunk = char[APPEND_CHUNK_SIZE];
  std::vector<TChunk *> buf;
  TChunk *t_current;
  uint64_t current_offset{0};
  uint64_t t_offset{0};

  AppendBuffer();

  ~AppendBuffer();

  uint64_t append(char c);
  uint64_t append(const char *text, uint64_t length);

  const char *read(uint64_t pos, uint64_t *out_len);
  inline uint64_t length();
};
