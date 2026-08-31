#include "internal/ui/text_mode.h"
#include "bed.h"

namespace bed::internal::ui {
TextMode::TextMode(BEd &bed) : bed(bed) {
  cursor = 0;
}

std::pair<vase::Shard *, bool> TextMode::run() {
  auto [row, col] = bed.io.cursor_position();
  auto [rows, cols] = bed.io.terminal_size();
  if (row > rows)
    throw fatal_error("Invalid cursor position.", 1);
  start = row;
  height = rows - row + 1;
  term_width = cols;
  term_height = rows;
  cmd.clear();
  cursor = 0;
  redraw();
  bool running = true;

  while (running) {
    io::KeyEvent res = bed.io.read_key();
    switch (res.type) {
    case io::KeyEvent::KeyType::EOF_:
      running = false;
      break;
    case io::KeyEvent::KeyType::MOUSE:
    case io::KeyEvent::KeyType::RESIZE:
      break;
    case io::KeyEvent::KeyType::CHAR:
      switch (res.modifier) {
      case io::KeyEvent::Modifier::SHIFT:
      case io::KeyEvent::Modifier::ALT:
      case io::KeyEvent::Modifier::CTRL_ALT:
      case io::KeyEvent::Modifier::CTRL:
        break;
      case io::KeyEvent::Modifier::NONE:
        if (res.text[0] == '\b' || res.text[0] == 0x7f) {
          if (cursor > 0) {
            cmd.erase(--cursor, 1);
          }
        } else if (res.text[0] == '\n') {
          cmd.insert(cursor++, 1, '\n');
          grow();
        } else {
          cmd.insert(cursor, res.text);
          cursor += res.text.size();
        }
        break;
      }
      break;
    case io::KeyEvent::KeyType::PASTE:
      cmd.insert(cursor, res.text);
      cursor += res.text.size();
      grow();
      break;
    case io::KeyEvent::KeyType::SPECIAL:
      switch (res.special_key) {
      case io::KeyEvent::SpecialKey::UNKNOWN:
        break;
      case io::KeyEvent::SpecialKey::RIGHT:
        if (cursor < cmd.size())
          ++cursor;
        break;
      case io::KeyEvent::SpecialKey::LEFT:
        if (cursor > 0)
          --cursor;
        break;
      case io::KeyEvent::SpecialKey::UP: {
        size_t line_start =
          cmd.rfind('\n', cursor == 0 ? 0 : cursor - 1);
        if (line_start == std::string::npos)
          line_start = 0;
        else
          ++line_start;
        size_t col = cursor - line_start;
        if (line_start == 0)
          break;
        size_t prev_end = line_start - 1;
        size_t prev_start =
          cmd.rfind('\n', prev_end == 0 ? 0 : prev_end - 1);
        if (prev_start == std::string::npos)
          prev_start = 0;
        else
          ++prev_start;
        size_t prev_len = prev_end - prev_start;
        cursor = prev_start + std::min(col, prev_len);
        break;
      }
      case io::KeyEvent::SpecialKey::DOWN: {
        size_t line_start =
          cmd.rfind('\n', cursor == 0 ? 0 : cursor - 1);
        if (line_start == std::string::npos)
          line_start = 0;
        else
          ++line_start;
        size_t col = cursor - line_start;
        size_t line_end = cmd.find('\n', cursor);
        if (line_end == std::string::npos)
          line_end = cmd.size();
        if (line_end == cmd.size())
          break;
        size_t next_start = line_end + 1;
        size_t next_end = cmd.find('\n', next_start);
        if (next_end == std::string::npos)
          next_end = cmd.size();
        size_t next_len = next_end - next_start;
        cursor = next_start + std::min(col, next_len);
        break;
      }
      case io::KeyEvent::SpecialKey::DELETE:
        if (cursor < cmd.size())
          cmd.erase(cursor, 1);
        break;
      }
      break;
    }
    redraw();
    if (cmd.size() >= 3 && cmd.compare(cmd.size() - 3, 3, "\n.\n") == 0) {
      cmd.erase(cmd.size() - 3);
      cursor = cmd.size();
      running = false;
    }
    if (cmd.size() == 2 && cmd.compare(0, 2, ".\n") == 0) {
      cmd.clear();
      cursor = 0;
      running = false;
    }
  }

  size_t total_lines = cmd.size() ? 1 + std::count(cmd.begin(), cmd.end(), '\n') : 0;
  uint16_t last_row = start + total_lines;
  bed.io.move_cursor(last_row, 1);
  bed.io.write("\n", 1);
  return {vase::Shard::from_string(cmd.data(), cmd.length(), true), false};
}

void TextMode::grow() {
  size_t required_height = 1 + std::count(cmd.begin(), cmd.end(), '\n');
  auto [rows, cols] = bed.io.terminal_size();
  term_height = rows;
  term_width = cols;
  if (required_height > height)
    height = required_height;
  long overflow = long(start) + long(height) - 1 - long(rows);
  if (overflow <= 0)
    return;
  bed.io.move_cursor(rows, 1);
  for (long i = 0; i < overflow; ++i)
    bed.io.write("\n", 1);
  start -= overflow;
  if (start < 1)
    start = 1;
}

void TextMode::redraw() {
  auto [rows, cols] = bed.io.terminal_size();
  term_height = rows;
  term_width = cols;
  for (uint16_t i = 0; i < height; ++i) {
    bed.io.move_cursor(start + i, 1);
    bed.io.write("\x1b[2K", 4);
  }
  size_t line = 0;
  size_t line_start = 0;
  for (size_t i = 0; i < cursor; ++i) {
    if (cmd[i] == '\n') {
      ++line;
      line_start = i + 1;
    }
  }
  size_t col = cursor - line_start;
  size_t pos = 0;
  uint16_t screen_line = start;
  while (pos <= cmd.size()) {
    size_t end = cmd.find('\n', pos);
    if (end == std::string::npos)
      end = cmd.size();
    if (screen_line < start + height) {
      size_t len = std::min(end - pos, size_t(term_width));
      bed.io.move_cursor(screen_line, 1);
      bed.io.write(cmd.substr(pos, len));
    }
    if (end == cmd.size())
      break;
    pos = end + 1;
    ++screen_line;
  }
  size_t vis_col = std::min(col, size_t(term_width - 1));
  bed.io.move_cursor(start + line, vis_col + 1);
}
} // namespace bed::internal::ui
