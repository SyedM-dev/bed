#include "bed.h"
#include "internal/io/io.h"

namespace bed::internal::io {
std::pair<std::string, bool> IO::get_command(BEd &ctx) {
  auto [row, col] = cursor_position();
  auto [rows, cols] = terminal_size();
  if (row > rows)
    throw fatal_error("Invalid cursor position.", 1);
  uint16_t start = row;
  uint16_t height = rows - row + 1;
  // uint16_t width = cols;
  // TODO: support multiline wrapped commands.
  // also handle ctrl+l to try and clear screen.

  std::string cmd;
  size_t cursor = 0;

  std::string prompt;
  if (ctx.prompt_mode)
    prompt = ctx.prompt(ctx);

  auto redraw = [&] {
    move_cursor(start, 1);
    write_all(STDOUT_FILENO, "\x1b[2K", 4);
    write_all(STDOUT_FILENO, prompt.c_str(), prompt.size());
    write_all(STDOUT_FILENO, cmd.c_str(), cmd.size());
    move_cursor(start, cursor + prompt.size() + 1);
  };
  redraw();

  bool eof = false;

  while (true) {
    KeyEvent ev = read_key();
    if (
      (ev.type == KeyEvent::KeyType::CHAR
       && ev.modifier == KeyEvent::Modifier::CTRL
       && ev.text[0] == 'd')
      || ev.type == KeyEvent::KeyType::NONE
    ) {
      eof = true;
      break;
    }
    if (ev.type == KeyEvent::KeyType::CHAR
        && ev.modifier == KeyEvent::Modifier::CTRL
        && ev.text[0] == 'w') {
      // TODO: delete previous word here.
      if (cursor == 0)
        continue;
      cmd.erase(cursor -= 1, 1);
      redraw();
      continue;
    }
    if (ev.type == KeyEvent::KeyType::CHAR
        && ev.modifier == KeyEvent::Modifier::NONE) {
      if (ev.text == "\r" || ev.text == "\n")
        break;
      if (ev.text.size() == 1
          && (ev.text[0] == '\x7f' || ev.text[0] == '\x08')) {
        if (cursor == 0)
          continue;
        cmd.erase(cursor -= 1, 1);
        redraw();
        continue;
      }
      cmd.insert(cursor, ev.text);
      cursor += ev.text.size();
      redraw();
      continue;
    }
    if (ev.type == KeyEvent::KeyType::SPECIAL) {
      if (ev.special_key == KeyEvent::SpecialKey::LEFT) {
        if (cursor == 0)
          continue;
        cursor -= 1;
      } else if (ev.special_key == KeyEvent::SpecialKey::RIGHT) {
        if (cursor >= cmd.size())
          continue;
        cursor += 1;
      } else if (ev.special_key == KeyEvent::SpecialKey::DELETE) {
        if (cursor >= cmd.size())
          continue;
        cmd.erase(cursor, 1);
      }
      redraw();
      continue;
    }
    if (ev.type == KeyEvent::KeyType::PASTE) {
      std::string text = ev.text;
      if (!text.empty() && text.front() == '\n')
        text.erase(text.begin());
      if (!text.empty() && text.back() == '\n')
        text.pop_back();
      bool multiline = text.find('\n') != std::string::npos;
      if (multiline) {
        size_t line_count = 1 + std::count(text.begin(), text.end(), '\n');
        if (height == 1) {
          write_all(STDOUT_FILENO, "\n", 1);
          --start;
          ++height;
        }
        bool go_ahead;
        std::string msg =
          "* paste "
          + std::to_string(line_count)
          + " lines into a single-line command? (y/n) ";
        move_cursor(start + 1, 1);
        write_all(STDOUT_FILENO, "\x1b[2K", 4);
        write_all(STDOUT_FILENO, msg.c_str(), msg.size());
        while (true) {
          KeyEvent ev = read_key();
          if (ev.type == KeyEvent::KeyType::CHAR && ev.text.size() == 1) {
            if (ev.text[0] == 'y' || ev.text[0] == 'Y') {
              go_ahead = true;
              break;
            }
            if (ev.text[0] == 'n' || ev.text[0] == 'N') {
              go_ahead = false;
              break;
            }
          }
          if (ev.type == KeyEvent::KeyType::NONE) {
            go_ahead = false;
            break;
          }
        }
        move_cursor(start + 1, 1);
        write_all(STDOUT_FILENO, "\x1b[2K", 4);
        if (!go_ahead) {
          redraw();
          continue;
        }
        std::replace(text.begin(), text.end(), '\n', ' ');
      }
      cmd.insert(cursor, text);
      cursor += text.size();
      redraw();
      continue;
    }
  }

  for (uint16_t i = 1; i < height; ++i) {
    move_cursor(start + i, 1);
    write_all(STDOUT_FILENO, "\x1b[2K", 4);
  }
  move_cursor(start, 1);
  write_all(STDOUT_FILENO, "\n", 1);
  return {cmd, eof};
}
} // namespace bed::internal::io
