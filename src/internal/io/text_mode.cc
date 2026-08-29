#include "bed.h"
#include "internal/io/io.h"

namespace bed::internal::io {
std::pair<std::string, bool> IO::get_text(BEd &) {
  uint16_t start;
  uint16_t height;
  {
    auto [row, col] = cursor_position();
    auto [rows, cols] = terminal_size();
    if (row > rows)
      throw fatal_error("Invalid cursor position.", 1);
    start = row;
    height = rows - row + 1;
  }
  enable_mouse();
  // uint16_t width = cols;

  disable_mouse();
  for (uint16_t i = 1; i < height; ++i) {
    move_cursor(start + i, 1);
    write_all(STDOUT_FILENO, "\x1b[2K", 4);
  }
  move_cursor(start, 1);
  write_all(STDOUT_FILENO, "\n", 1);
  return {"", true};
}
} // namespace bed::internal::io
