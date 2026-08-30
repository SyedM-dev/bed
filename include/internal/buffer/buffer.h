#pragma once

#include "definitions.h"
#include "internal/syntax/parser.h"
#include "internal/syntax/ruby/parser.h"
#include "internal/theme/theme.h"
#include "pch.h"

namespace bed::internal::buffer {
struct Buffer {
  enum {
    Unmodified,
    Modified,
    Warned
  } state;
  std::filesystem::path save_path = "";

  vase::Shard *root;

  std::string name;
  std::string language;
  std::optional<syntax::Parser> parser;

  explicit Buffer(std::string name);
  ~Buffer();

  uint64_t lines();
  uint64_t bytes();
  void load(BEd &ctx, vase::Shard *text);
  vase::Shard *copy(uint64_t start_line, uint64_t end_line);
  void substitute(
    BEd &ctx, uint64_t start_line, uint64_t end_line,
    std::string &regex, std::string &replacement, std::string &options
  );
  void join(BEd &ctx, uint64_t start_line, uint64_t end_line);
  void remove(BEd &ctx, uint64_t start_line, uint64_t end_line);
  void append(BEd &ctx, vase::Shard *text, uint64_t line);
  void print(BEd &ctx, uint64_t start_line, uint64_t end_line);
  void number_print(BEd &ctx, uint64_t start_line, uint64_t end_line);
  std::string list_string(std::string_view s);
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
