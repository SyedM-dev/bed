#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::io {
struct KeyEvent {
  enum struct KeyType {
    NONE,
    CHAR,
    SPECIAL,
    MOUSE,
    PASTE
  };
  enum struct SpecialKey {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    DELETE,
    UNKNOWN
  };
  enum struct Modifier {
    NONE,
    SHIFT,
    ALT,
    CTRL,
    CTRL_ALT
  };
  enum struct MouseState {
    PRESS,
    RELEASE
  };

  KeyType type = KeyType::NONE;
  std::string text;

  SpecialKey special_key = SpecialKey::UNKNOWN;
  Modifier modifier = Modifier::NONE;

  MouseState mouse_state = MouseState::PRESS;
  uint16_t mouse_x = 0;
  uint16_t mouse_y = 0;
};

struct IO {
  IO();
  ~IO();

  IO(const IO &) = delete;
  IO &operator=(const IO &) = delete;

  void enable_mouse();
  void disable_mouse();

  std::pair<uint16_t, uint16_t> terminal_size();
  std::pair<uint16_t, uint16_t> cursor_position();
  void move_cursor(uint16_t row, uint16_t col);

  std::pair<std::string, bool> get_command(BEd &);

  KeyEvent read_key();

private:
  static termios orig_termios;
  static bool cleaned;
  static void cleanup();

  std::deque<char> input_queue;

  bool get_next_byte(char &out);
  void enqueue_bytes(const std::string &bytes);
  static int utf8_seq_len(uint8_t byte);

  std::string read_next_unit();
  bool read_bracketed_paste(std::string &out);

  KeyEvent parse_mouse(const std::string &buf);
  KeyEvent parse_escape(const std::string &buf);
};
} // namespace bed::internal::io
