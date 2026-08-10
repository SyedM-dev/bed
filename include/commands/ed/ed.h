#pragma once

#include "internal/vase/vase.h"
#include "pch.h"

namespace crib::commands::ed {
struct ed_error : std::runtime_error {
  ed_error(const char *msg) : std::runtime_error(msg) {}
};

struct Command {
  enum struct Type : uint8_t {
    Invalid,
    Quit,
    ForceQuit,
    PromptToggle,
    HelpToggle,
    Help,
    Print,
    List,
    Number,
    Append,
    Insert,
    Change,
    Delete,
    Write,
    Join,
    Mark,
    Edit,
    ForceEdit,
    Filename,
    Dump,
    None
  } type;

  struct Address {
    enum struct Type : uint8_t {
      None,
      Current,
      Last,
      Number,
      Mark,
      SearchForward,
      SearchBackward
    } type;
    int64_t number = 0;
    std::string regex = "";
    int64_t offset = 0;
    char mark = '\0';
  };

  Address start{};
  Address end{};
  static constexpr uint8_t SEMICOLON = 0b01;
  static constexpr uint8_t RANGE = 0b10;
  uint8_t address_flags = 0;
  char suffix = '\0';
  std::string argument;
};

struct Ed {
  crib::internal::vase::Vase vase;
  uint64_t line = 0;
  bool modified = false;
  bool quitting = false;
  bool editing = false;
  uint64_t marks[26]{0};
  std::string last_regex = "";
  bool help_mode = false;
  bool suppress_mode = false;
  bool prompt_mode = false;
  std::string prompt = "*";
  std::filesystem::path save_path = "";
  std::string last_message = "";

  Ed(std::string file_path, bool suppress_mode, std::string prompt_)
      : vase("/tmp"), suppress_mode(suppress_mode), prompt(prompt_) {
    if (prompt == "")
      prompt = "*";
    else
      prompt_mode = true;
    if (file_path != "")
      handle("E " + file_path, false);
  }

  void parse_address(std::string_view cmd, uint64_t &i, Command::Address &addr);
  void resolve_address(Command::Address addr, uint64_t *out_line);
  Command parse(std::string cmd, bool eof);
  void append(std::string text, uint64_t line);
  void remove(uint64_t start_line, uint64_t end_line);
  std::string list_string(std::string_view s);
  void join(uint64_t start_line, uint64_t end_line);
  bool handle(std::string cmd, bool eof);
};

std::string summary();
void help();
void run(std::vector<std::string>);
} // namespace crib::commands::ed
