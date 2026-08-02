#pragma once

#include "buffer.h"
#include "pch.h"

struct AppendBuffer : Buffer {
  static constexpr uint32_t CHUNK_SIZE = 1 << 16;

  using TChunk = char[CHUNK_SIZE];
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

  inline void new_text_chunk();
};
