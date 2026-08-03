#pragma once

#include "../constants.h"
#include "buffer.h"
#include "pch.h"

struct AppendBuffer : Buffer {
  using TChunk = char[APPEND_CHUNK_SIZE];
  std::vector<TChunk *> buf;
  TChunk *t_current;
  uint32_t current_offset{0};
  uint32_t t_offset{0};

  AppendBuffer();

  ~AppendBuffer();

  uint32_t key(char c);
  uint32_t append(const char *text, uint32_t length);

  const char *read(uint32_t pos, uint32_t *out_len);
  inline uint32_t length();
};
