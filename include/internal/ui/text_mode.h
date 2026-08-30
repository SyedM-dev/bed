#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::ui {
struct TextMode {
  std::string cmd;
  uint16_t cursor;
  uint16_t start;
  uint16_t height;
  uint16_t term_height;
  uint16_t term_width;
  BEd &bed;

  TextMode(BEd &);
  std::pair<vase::Shard *, bool> run();
  void grow(size_t required_height);
  void redraw();
};
} // namespace bed::internal::ui
