#pragma once

#include "definitions.h"
#include "internal/marks/marks.h"
#include "internal/syntax/parser.h"
#include "internal/syntax/ruby/parser.h"
#include "internal/theme/theme.h"
#include "pch.h"

namespace bed::internal::buffer {
struct Buffer {
  vase::Vase vase;
  marks::MarksEngine marks;
  syntax::Language lang;
  syntax::Parser parser;
  uint64_t line = 0;
  bool modified;
  std::filesystem::path save_path = "";
  struct {
    uint64_t start{0};
    uint64_t end{0};
  } prev_range;

  Buffer();
  Buffer(std::string command);
  Buffer(std::filesystem::path path);

  ~Buffer() = default;

  void load(std::string command);
  void load(std::filesystem::path path);
  void jump(uint64_t n_line);
  void join(uint64_t start_line, uint64_t end_line);
  void remove(uint64_t start_line, uint64_t end_line);
  void append(std::string text, uint64_t line);
  void print(BEd &ctx, uint64_t start_line, uint64_t end_line);
  std::string list_string(std::string_view s);
};
} // namespace bed::internal::buffer
