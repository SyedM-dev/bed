#pragma once

#include "../decl.h"
#include "shard.h"

namespace bed::internal::buffer {
struct HistoryBuffer : ShardBuffer {
  HistoryBuffer(
    std::string name, vase::Shard *root,
    const syntax::ParserSnapshot &snapshot
  ) : ShardBuffer(std::move(name), root, snapshot, Kind::History) {}

  bool waste() override {
    return true;
  }
  void load(BEd &, vase::Shard *) override {
    throw ed_error("History buffers are read-only.");
  }
  void set_filename(std::filesystem::path) override {}
  std::filesystem::path filename() override {
    return {};
  }
  void substitute(BEd &, uint64_t, uint64_t, std::string &, std::string &, std::string &) override {
    throw ed_error("History buffers are read-only.");
  }
  void join(BEd &, uint64_t, uint64_t) override {
    throw ed_error("History buffers are read-only.");
  }
  void remove(BEd &, uint64_t, uint64_t) override {
    throw ed_error("History buffers are read-only.");
  }
  void append(BEd &, vase::Shard *, uint64_t) override {
    throw ed_error("History buffers are read-only.");
  }
  void replace(BEd &, vase::Shard *, uint64_t, uint64_t) override {
    throw ed_error("History buffers are read-only.");
  }
};
} // namespace bed::internal::buffer
