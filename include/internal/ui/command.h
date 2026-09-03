#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::ui {
struct CommandIO {
  std::string cmd;
  uint16_t cursor;
  std::string prompt;
  uint16_t start;
  uint16_t height;
  uint16_t term_height;
  uint16_t term_width;
  BEd &bed;

  CommandIO(BEd &);
  std::pair<std::string, bool> run();
  std::pair<std::string, bool> run_pipe();
  std::pair<std::string, bool> run_terminal();
  void redraw();
};
} // namespace bed::internal::ui
