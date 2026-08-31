#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::io {
struct KeyEvent {
  enum struct ReadResult {
    SUCCESS,
    EOF_,
    RESIZE
  };
  enum struct KeyType {
    EOF_,
    CHAR,
    SPECIAL,
    MOUSE,
    PASTE,
    RESIZE
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

  KeyType type = KeyType::EOF_;
  std::string text;

  SpecialKey special_key = SpecialKey::UNKNOWN;
  Modifier modifier = Modifier::NONE;

  MouseState mouse_state = MouseState::PRESS;
  uint16_t mouse_x = 0;
  uint16_t mouse_y = 0;
};

struct IO {
  static termios orig_termios;
  static termios raw_termios;
  static bool cleaned;
  static void cleanup();
  static void enable_raw();
  static volatile std::atomic_bool resized;
  static void handle_sigwinch(int);

  IO();
  ~IO();

  IO(const IO &) = delete;
  IO &operator=(const IO &) = delete;

  void enable_mouse();
  void disable_mouse();

  std::pair<uint16_t, uint16_t> terminal_size();
  std::pair<uint16_t, uint16_t> cursor_position();
  void move_cursor(uint16_t row, uint16_t col);

  KeyEvent read_key();
  void write(const char *, uint64_t);
  void write(std::string_view);
  void run_pty(const std::string &);

  std::deque<char> input_queue;

  KeyEvent::ReadResult get_next_byte(char &out);
  void enqueue_bytes(const std::string &bytes);
  static int utf8_seq_len(uint8_t byte);

  KeyEvent::ReadResult read_next_unit(std::string &out);
  KeyEvent::ReadResult read_bracketed_paste(std::string &out);

  KeyEvent parse_mouse(const std::string &buf);
  KeyEvent parse_escape(const std::string &buf);
};
} // namespace bed::internal::io
