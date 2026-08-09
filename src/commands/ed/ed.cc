#include "commands/ed/ed.h"
#include "cli.h"

namespace crib::commands::ed {
std::string summary() {
  return "A POSIX-compliant line editor.";
}

void help() {}

void run(std::vector<std::string> args) {
  std::string prompt = "";
  std::string file;
  bool suppress = false;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p") {
      i++;
      if (i >= args.size())
        throw crib::cli::cli_error("Prompt not specified!", 1);
      prompt = args[i];
    } else if (args[i] == "-s") {
      suppress = true;
    } else {
      if (file.size())
        throw crib::cli::cli_error("Invalid arguments given.", 1);
      file = args[i];
    }
  }
  Ed ed(file, suppress, prompt);
  std::string command;
  while (true) {
    std::string cmd;
    if (ed.prompt_mode)
      std::cout << ed.prompt;
    bool eof = !std::getline(std::cin, cmd);
    if (!ed.handle(cmd, eof))
      return;
  }
}
} // namespace crib::commands::ed
