#include "internal/ui/command.h"
#include "bed.h"

namespace bed::internal::ui {
/*template <typename F>
static void for_each_cluster(std::string_view s, F &&f) {
  unicode_width_state_t state;
  unicode_width_init(&state);
  size_t i = 0;
  while (i < s.size()) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    size_t bytes = 1;
    int width = 0;
    if (c < 128) {
      width = unicode_width_process(&state, c);
    } else {
      uint_least32_t cp;
      size_t decoded = grapheme_decode_utf8(s.data() + i, s.size() - i, &cp);
      bytes = decoded > 0 ? decoded : 1;
      width = unicode_width_process(&state, cp);
    }
    if (width < 0)
      width = 0;
    f(i, bytes, width);
    i += bytes;
  }
}

static int display_width(std::string_view s) {
  int w = 0;
  for_each_cluster(s, [&](size_t, size_t, int cw) { w += cw; });
  return w;
}

static uint16_t count_clusters(std::string_view s) {
  uint16_t n = 0;
  for_each_cluster(s, [&](size_t, size_t, int) { ++n; });
  return n;
}

static std::vector<uint16_t> wrap_offsets(std::string_view line, uint16_t avail) {
  std::vector<uint16_t> offsets{0};
  int col = 0;
  for_each_cluster(line, [&](uint16_t i, uint16_t, int w) {
    if (col + w > avail && col > 0) {
      offsets.push_back(i);
      col = 0;
    }
    col += w;
  });
  return offsets;
}

// The word under/before `byte_pos`, split on plain ASCII spaces. Used to
// pick what prefix to hand the suggestion trie.
// TODO: change to use libgrapheme word break here.
static std::string current_word(const std::string &line, size_t byte_pos) {
  size_t start = (byte_pos == 0) ? std::string::npos : line.rfind(' ', byte_pos - 1);
  start = (start == std::string::npos) ? 0 : start + 1;
  if (byte_pos < start)
    byte_pos = start;
  return line.substr(start, byte_pos - start);
}*/

CommandIO::CommandIO(BEd &bed) : bed(bed) {
  if (bed.prompt_mode)
    prompt = bed.prompt(bed);
  cursor = 0;
}

std::pair<std::string, bool> CommandIO::run() {
  if (!bed.io.interactive())
    return run_pipe();
  return run_terminal();
}

std::pair<std::string, bool> CommandIO::run_pipe() {
  bed.io.write(prompt);
  return bed.io.read_pipe();
}

std::pair<std::string, bool> CommandIO::run_terminal() {
  auto [row, col] = bed.io.cursor_position();
  auto [rows, cols] = bed.io.terminal_size();
  if (row > rows)
    throw fatal_error("Invalid cursor position.", 1);
  start = row;
  height = rows - row + 1;
  term_width = cols;
  term_height = rows;
  redraw();

  io::KeyEvent res;
  bool running = true;
  while (running) {
    res = bed.io.read_key();
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
          if (cursor > 0)
            cmd.erase(--cursor, 1);
        } else if (res.text[0] == '\n') {
          running = false;
        } else {
          cmd.insert(cursor++, res.text);
        }
        break;
      }
      break;
    case io::KeyEvent::KeyType::PASTE:
      cmd.insert(cursor, res.text);
      cursor += res.text.size();
      break;
    case io::KeyEvent::KeyType::SPECIAL:
      switch (res.special_key) {
      case io::KeyEvent::SpecialKey::UNKNOWN:
      case io::KeyEvent::SpecialKey::UP:
      case io::KeyEvent::SpecialKey::DOWN:
        break;
      case io::KeyEvent::SpecialKey::RIGHT:
        if (cursor < cmd.size())
          cursor++;
        break;
      case io::KeyEvent::SpecialKey::LEFT:
        if (cursor > 0)
          cursor--;
        break;
      case io::KeyEvent::SpecialKey::DELETE:
        if (cursor < cmd.size())
          cmd.erase(cursor, 1);
        break;
      }
      break;
    }
    redraw();
  }

  for (uint16_t i = 1; i < height; ++i) {
    bed.io.move_cursor(start + i, 1);
    bed.io.write("\x1b[2K", 4);
  }
  bed.io.move_cursor(start, 1);
  bed.io.write("\n", 1);
  return {cmd, false};
}

void CommandIO::redraw() {
  bed.io.move_cursor(start, 1);
  bed.io.write("\x1b[2K", 4);
  bed.io.move_cursor(start, 1);
  bed.io.write(prompt);
  bed.io.write(cmd);
  bed.io.move_cursor(start, prompt.size() + cursor + 1);
}
} // namespace bed::internal::ui
