#pragma once

#include "definitions.h"
#include "internal/buffer/buffer.h"
#include "internal/functions/functions.h"
#include "internal/functions/suffixes.h"
#include "internal/io/io.h"
#include "internal/marks/marks.h"
#include "internal/theme/theme.h"
#include "internal/ui/command.h"
#include "internal/ui/text_mode.h"
#include "pch.h"

namespace bed {
struct BEd {
  internal::trie::Trie<internal::functions::Function> functions;
  internal::functions::Function no_op;
  internal::functions::Function eof_op;
  std::array<std::optional<internal::functions::Suffix>, 26> suffixes;
  internal::theme::Theme theme;
  std::unordered_map<std::string, internal::syntax::Language> languages;
  internal::io::IO &io;
  internal::vase::AppendStorage append{"/tmp"};

  bool help_mode = false;
  std::string last_help = "";
  bool prompt_mode = true;
  std::function<std::string(BEd &)> prompt = nullptr;
  bool suppress_mode = false;
  bool temporary_current = false;
  std::string last_regex = "";
  std::string last_symbol = "";
  std::string last_replacement = "";

  std::unordered_map<std::string, internal::buffer::Buffer *> buffers;

  internal::buffer::Range prev;
  internal::marks::MarksEngine marks;

  BEd(std::vector<std::string> args, internal::io::IO &io);
  ~BEd();

  internal::buffer::Buffer &buffer(const std::string &);
  internal::buffer::Line &current();
  void mark(char, internal::buffer::Line);

  void handle(std::string_view cmd, bool eof);
  void run();
  void suffix_handle(char s);
};
} // namespace bed
