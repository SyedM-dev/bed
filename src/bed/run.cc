#include "bed.h"

namespace bed {
BEd::BEd(std::vector<std::string> args, internal::io::IO &io)
    : theme(internal::theme::Theme::default_theme()), io(io) {
  internal::commands::Command::register_posix(*this);
  internal::commands::Suffix::register_suffixes(*this);
  std::string prompt_ = "";
  std::string file = "";
  bool suppress = false;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p") {
      i++;
      if (i >= args.size())
        throw fatal_error("Prompt not specified!", 1);
      prompt_ = args[i];
    } else if (args[i] == "-s") {
      suppress = true;
    } else {
      if (file.size())
        throw fatal_error("Invalid arguments given.", 1);
      file = args[i];
    }
  }
  if (prompt_ != "")
    prompt = [p = std::move(prompt_)](BEd &) { return p; };
  else
    prompt_mode = false;
  suppress_mode = suppress;
  active = new internal::buffer::Buffer();
  buffers.emplace("0", active);
  if (file != "")
    handle("E " + file, false);
}

BEd::~BEd() {
  for (auto &[_, buffer] : buffers)
    delete buffer;
}

void BEd::run() {
  while (true) {
    auto [cmd, eof] = io.get_command(*this);
    try {
      handle(cmd, eof);
    } catch (ed_error &e) {
      std::cout << e.what() << std::endl;
    }
  }
}
} // namespace bed
