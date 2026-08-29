#pragma once

#include "definitions.h"
#include "internal/buffer/buffer.h"
#include "internal/trie/trie.h"
#include "pch.h"

namespace bed::internal::functions {
struct Function {
  struct GlobalArg {
    char delim;
    std::string str;
  };
  struct RegexArg {
    std::string expression;
    std::string replacement;
    std::string options;
  };
  struct ShellArg {
    std::string cmd;
  };
  struct RubyArg {
    std::string cmd;
  };
  using Argument = std::variant<
    std::monostate,
    GlobalArg,
    RegexArg,
    ShellArg,
    RubyArg,
    std::string,
    std::filesystem::path,
    int64_t,
    char,
    buffer::Range,
    buffer::Line>;

  enum struct AddressKind {
    None,
    Line,
    Range
  } address_kind;

  enum struct ArgumentKind {
    None,
    Regex,
    Shell,
    Ruby,
    Any,
    File,
    Number,
    Mark,
    Range,
    Line,
    Global
  } argument_kind;

  enum struct InputMode {
    None,
    Text,
    Interactive,
    CommandList
  } input_mode;

  std::string desc;
  std::string default_address;
  bool accept_zero;

  std::function<
    std::tuple<vase::Shard *, syntax::Language *, void *>(
      BEd &ctx, const buffer::Address &addr, const Argument &arg
    )>
    pre_text_mode;

  std::function<
    void(
      BEd &ctx, const buffer::Address &addr,
      vase::Shard *text, const Argument &arg,
      std::vector<buffer::Line> *marked
    )>
    handle;

  static void register_posix(BEd &ctx);
};
} // namespace bed::internal::functions
