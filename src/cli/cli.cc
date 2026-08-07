#include "cli.h"
#include "commands/ed/ed.h"

namespace crib::cli {
static const std::unordered_map<std::string, Command> commands = {
  {"ed",
   {crib::commands::ed::run,
    crib::commands::ed::summary,
    crib::commands::ed::help}},
};

bool use_color() {
  if (std::getenv("NO_COLOR"))
    return false;
  if (std::getenv("CLICOLOR_FORCE"))
    return true;
  if (!isatty(STDERR_FILENO))
    return false;
  const char *term = std::getenv("TERM");
  if (!term || std::string_view(term) == "dumb" || std::string_view(term) == "xterm")
    return false;
  if (const char *clicolor = std::getenv("CLICOLOR"))
    return std::string_view(clicolor) != "0";
  return true;
}

bool use_colors = use_color();

void help() {
  std::cout << "Usage: crib <command> [options]\n\n"
            << "Where command is one of\n";
  for (auto &[name, cmd] : commands)
    std::cout << "  " << name << " - " << cmd.summary() << "\n";
}

int execute(const std::string &name, const Command &cmd, int argc, char *argv[]) {
  try {
    std::vector<std::string> args(argv, argv + argc);
    cmd.run(args);
    return 0;
  } catch (const cli_error &e) {
    std::cerr << "Error running " << name << ": " << e.what() << '\n';
    return e.exit_code;
  } catch (const std::exception &e) {
    std::cerr << "Error running " << name << ": " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown error running " << name << '\n';
    return 1;
  }
}

int run(int argc, char *argv[]) {
  std::string invoked_as = std::filesystem::path(argv[0]).filename();

  if (auto it = commands.find(invoked_as); it != commands.end())
    return execute(it->first, it->second, argc, argv);

  if (argc > 1)
    if (auto it = commands.find(argv[1]); it != commands.end())
      return execute(it->first, it->second, argc - 1, argv + 1);

  if (argc > 1 && std::string(argv[1]) == "--help") {
    help();
    return 0;
  }

  std::cerr << "Invalid command given.\n\n";
  help();
  return 2;
}
} // namespace crib::cli
