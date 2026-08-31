#pragma once

#include "decl.h"
#include "pch.h"

namespace bed::internal::buffer {
struct ClipBuffer : Buffer {
  ClipBuffer(std::string name)
      : Buffer(name, Kind::Clip) {};
  ~ClipBuffer();

  void clip_write(vase::Shard *text);
  bool waste() override;
  uint64_t lines() override;
  uint64_t bytes() override;
  void load(BEd &ctx, vase::Shard *text) override;
  void set_filename(std::filesystem::path path) override;
  std::filesystem::path filename() override;
  vase::Shard *copy(uint64_t start_line, uint64_t end_line) override;
  void substitute(
    BEd &ctx, uint64_t start_line, uint64_t end_line,
    std::string &regex, std::string &replacement, std::string &options
  ) override;
  void join(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void remove(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void append(BEd &ctx, vase::Shard *text, uint64_t line) override;
  void replace(BEd &ctx, vase::Shard *text, uint64_t start_line, uint64_t end_line) override;
  void print(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void list_print(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  uint64_t next_closing(uint64_t start) override;
  uint64_t prev_closing(uint64_t start) override;
  uint64_t find_next(std::string_view pattern, uint64_t start) override;
  uint64_t find_prev(std::string_view pattern, uint64_t start) override;
};
} // namespace bed::internal::buffer
