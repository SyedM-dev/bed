#pragma once

#include "definitions.h"
#include "internal/buffer/buffer.h"
#include "internal/commands/commands.h"
#include "internal/commands/suffixes.h"
#include "internal/marks/marks.h"
#include "pch.h"

namespace bed {
struct BEd {
  internal::trie::Trie<internal::commands::Command> commands;
  internal::commands::Command no_op;
  internal::commands::Command eof_op;
  std::array<std::optional<internal::commands::Suffix>, 26> suffixes;

  bool help_mode = false;
  std::string last_help = "";
  bool prompt_mode = true;
  std::function<std::string(BEd &)> prompt = nullptr;
  bool suppress_mode = false;
  std::string last_regex = "";

  std::unordered_map<std::string, internal::buffer::Buffer *> buffers;
  internal::buffer::Buffer *active;

  BEd(std::vector<std::string> args);
  ~BEd();
  void handle(std::string_view cmd, bool eof);
  void run();
};
} // namespace bed
