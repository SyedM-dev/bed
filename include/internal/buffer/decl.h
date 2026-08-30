#pragma once

#include "definitions.h"
#include "internal/syntax/parser.h"
#include "internal/syntax/ruby/parser.h"
#include "internal/theme/theme.h"
#include "pch.h"

namespace bed::internal::buffer {
struct Buffer {
  enum struct Kind : uint8_t {
    Generic,
    Null,
    Clip,
    Cancel,
    Shell,
  } kind;

  enum : uint8_t {
    Special,
    Unmodified,
    Modified,
    Warned
  } state;

  std::string name;

  explicit Buffer(std::string name, Kind kind)
      : kind(kind), state(Unmodified), name(name) {};
  virtual ~Buffer() = default;

  virtual bool waste() = 0;
  virtual uint64_t lines() = 0;
  virtual uint64_t bytes() = 0;
  virtual void load(BEd &ctx, vase::Shard *text) = 0;
  virtual void load(BEd &ctx, const char *cmd) = 0;
  virtual void load(BEd &ctx, std::filesystem::path path) = 0;
  virtual void load(BEd &ctx) = 0;
  virtual vase::Shard *copy(uint64_t start_line, uint64_t end_line) = 0;
  virtual void substitute(
    BEd &ctx, uint64_t start_line, uint64_t end_line,
    std::string &regex, std::string &replacement, std::string &options
  ) = 0;
  virtual void join(BEd &ctx, uint64_t start_line, uint64_t end_line) = 0;
  virtual void remove(BEd &ctx, uint64_t start_line, uint64_t end_line) = 0;
  virtual void append(BEd &ctx, vase::Shard *text, uint64_t line) = 0;
  virtual void print(BEd &ctx, uint64_t start_line, uint64_t end_line) = 0;
  virtual void number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) = 0;
  virtual uint64_t find_next(std::string_view pattern, uint64_t start) = 0;
  virtual uint64_t find_prev(std::string_view pattern, uint64_t start) = 0;
  virtual uint64_t next_closing(uint64_t start) = 0;
  virtual uint64_t prev_closing(uint64_t start) = 0;
};

struct Line {
  std::string buffername;
  uint64_t number;
};

struct Range {
  std::string buffername;
  uint64_t start;
  uint64_t end;

  explicit Range(const Line &a, const Line &b) {
    if (a.buffername != b.buffername)
      throw ed_error("Invalid range.");
    if (a.number > b.number)
      throw ed_error("Invalid range.");
    buffername = a.buffername;
    start = a.number;
    end = b.number;
  };

  explicit Range(std::string buffername, uint64_t start, uint64_t end)
      : buffername(buffername), start(start), end(end) {
    if (start > end)
      throw ed_error("Invalid range.");
  };

  explicit Range() : buffername("default"), start(0), end(0) {};
};

using Address = std::variant<
  std::string,
  buffer::Range,
  buffer::Line>;
} // namespace bed::internal::buffer
