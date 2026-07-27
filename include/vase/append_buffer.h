#pragma once

#include "pch.h"

struct AppendBuffer {
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

  inline void key(char c);
  void append(const char *text, uint32_t length);

private:
  inline void new_text_chunk();
  inline void new_line_chunk();

  inline void _append(const char *text, uint32_t length);
};
