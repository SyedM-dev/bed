#pragma once

#include "../decl.h"

namespace bed::internal::buffer {
struct ShardBuffer : Buffer {
  vase::Shard *root;
  syntax::ParserSnapshot parse{};

  ShardBuffer(std::string name, vase::Shard *root, syntax::Language *language, Kind kind)
      : Buffer(std::move(name), kind), root(root) {
    vase::Shard::retain(root);
    if (language)
      parse = syntax::make_parser(root, lines(), language);
  }
  ShardBuffer(
    std::string name,
    vase::Shard *root,
    const syntax::ParserSnapshot &snapshot,
    Kind kind
  ) : Buffer(std::move(name), kind),
      root(root),
      parse(syntax::retain(snapshot)) {
    vase::Shard::retain(root);
  }
  ~ShardBuffer() {
    vase::Shard::release(root);
    syntax::release(parse);
  }

  uint64_t lines() override;
  uint64_t bytes() override;
  vase::Shard *copy(uint64_t start_line, uint64_t end_line) override;
  void print(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void list_print(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  uint64_t next_closing(uint64_t start) override;
  uint64_t prev_closing(uint64_t start) override;
  uint64_t find_next(std::string_view pattern, uint64_t start) override;
  uint64_t find_prev(std::string_view pattern, uint64_t start) override;
};
} // namespace bed::internal::buffer
