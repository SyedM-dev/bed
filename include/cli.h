#pragma once

#include "pch.h"

namespace crib::cli {
struct Command {
  void (*run)(std::vector<std::string>);
  std::string (*summary)();
  void (*help)();
};

struct cli_error : std::runtime_error {
  int exit_code;
  cli_error(std::string msg, int code)
      : std::runtime_error(std::move(msg)), exit_code(code) {}
};

extern bool use_colors;
void help();
int execute(const std::string &name, const Command &cmd, int argc, char *argv[]);
int run(int argc, char *argv[]);
} // namespace crib::cli
