#pragma once

#include "../decl.h"
#include "history.h"
#include "shard.h"

namespace bed::internal::buffer {
struct HistoryItem {
  syntax::ParserSnapshot parse_state;
  vase::Shard *text;
  std::chrono::system_clock::time_point timestamp;
  std::string summary;
};

struct GenericBuffer : ShardBuffer {
  uint64_t base_version{0};
  std::chrono::system_clock::time_point timestamp;
  std::string action;
  std::filesystem::path save_path{};
  std::vector<HistoryItem> undo_stack;
  std::vector<HistoryItem> redo_stack;

  GenericBuffer(std::string name)
      : ShardBuffer(name, nullptr, nullptr, Kind::Generic) {}
  ~GenericBuffer();

  void list_history(BEd &ctx);
  HistoryBuffer *get_history(uint64_t version);
  void snapshot(std::string action);
  bool undo(BEd &ctx);
  bool redo(BEd &ctx);
  void prune(int = 0);
  bool waste() override;
  void load(BEd &ctx, vase::Shard *text) override;
  void set_filename(std::filesystem::path path) override;
  std::filesystem::path filename() override;
  void substitute(
    BEd &ctx, uint64_t start_line, uint64_t end_line,
    std::string &regex, std::string &replacement, std::string &options
  ) override;
  void join(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void remove(BEd &ctx, uint64_t start_line, uint64_t end_line) override;
  void append(BEd &ctx, vase::Shard *text, uint64_t line) override;
  void replace(BEd &ctx, vase::Shard *text, uint64_t start_line, uint64_t end_line) override;
};
} // namespace bed::internal::buffer
