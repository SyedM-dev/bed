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

  using LChunk = uint32_t[CHUNK_SIZE];
  std::vector<LChunk *> newlines;
  LChunk *l_current;
  uint32_t l_offset{0};

  AppendBuffer();

  ~AppendBuffer();

  uint32_t key(char c);
  uint32_t append(const char *text, uint32_t length, uint32_t *line_count);

  const char *read(uint32_t pos, uint32_t *out_len);
  uint32_t count_lines(uint32_t pos, uint32_t length);

private:
  inline void new_text_chunk();
  inline void new_line_chunk();

  inline uint32_t find_newline(uint32_t target, uint32_t low, uint32_t high);

  inline uint32_t _append(const char *text, uint32_t length);
};
