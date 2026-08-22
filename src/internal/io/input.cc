#include "internal/io/io.h"

namespace bed::internal::io {
bool IO::get_next_byte(char &out) {
  if (!input_queue.empty()) {
    out = input_queue.front();
    input_queue.pop_front();
    return true;
  }
  return read(STDIN_FILENO, &out, 1) == 1;
}

void IO::enqueue_bytes(const std::string &bytes) {
  input_queue.insert(input_queue.begin(), bytes.begin(), bytes.end());
}

int IO::utf8_seq_len(uint8_t byte) {
  if ((byte & 0x80) == 0x00)
    return 1;
  if ((byte & 0xE0) == 0xC0)
    return 2;
  if ((byte & 0xF0) == 0xE0)
    return 3;
  if ((byte & 0xF8) == 0xF0)
    return 4;
  return 1;
}

std::string IO::read_next_unit() {
  std::string buf;
  char header;
  if (!get_next_byte(header))
    return buf;
  if (header == '\x1b') {
    buf.push_back(header);
    char c;
    if (!get_next_byte(c))
      return buf;
    buf.push_back(c);
    if (c != '[')
      return buf;
    if (!get_next_byte(c))
      return buf;
    buf.push_back(c);
    if (c == 'M') {
      for (int i = 0; i < 3; ++i) {
        if (!get_next_byte(c))
          break;
        buf.push_back(c);
      }
      return buf;
    }
    while ((uint8_t)c < 0x40 || (uint8_t)c > 0x7E) {
      if (buf.size() >= 32)
        break;
      if (!get_next_byte(c))
        break;
      buf.push_back(c);
    }
    return buf;
  }
  int seq_len = utf8_seq_len((uint8_t)header);
  buf.push_back(header);
  if (seq_len == 1)
    return buf;
  for (int i = 1; i < seq_len; i++) {
    char c;
    if (!get_next_byte(c)) {
      enqueue_bytes(buf);
      return {};
    }
    buf.push_back(c);
  }
  uint32_t prev_cp, cur_cp;
  grapheme_decode_utf8(buf.data(), buf.size(), &prev_cp);
  uint16_t state = 0;
  while (true) {
    char next_header;
    if (!get_next_byte(next_header))
      break;
    int next_len = utf8_seq_len((uint8_t)next_header);
    std::string next_seq(1, next_header);
    bool complete = true;
    for (int i = 1; i < next_len; i++) {
      char c;
      if (!get_next_byte(c)) {
        complete = false;
        break;
      }
      next_seq.push_back(c);
    }
    if (!complete) {
      enqueue_bytes(next_seq);
      break;
    }
    grapheme_decode_utf8(next_seq.data(), next_seq.size(), &cur_cp);
    if (grapheme_is_character_break(prev_cp, cur_cp, &state)) {
      enqueue_bytes(next_seq);
      break;
    }
    buf += next_seq;
    prev_cp = cur_cp;
  }
  return buf;
}

bool IO::read_bracketed_paste(std::string &out) {
  out.clear();
  std::string window;
  while (true) {
    char c;
    if (!get_next_byte(c))
      return false;
    window.push_back(c);
    if (window.size() == 5 && window == "\x1b[201") {
      char tilde;
      if (!get_next_byte(tilde))
        return false;
      if (tilde == '~')
        return true;
      out += window;
      out.push_back(tilde);
      window.clear();
      continue;
    }
    if (window.size() == 5) {
      out.push_back(window.front());
      window.erase(window.begin());
    }
  }
}

KeyEvent IO::parse_mouse(const std::string &buf) {
  KeyEvent ev;
  if (buf.size() < 6)
    return ev;
  uint8_t code = (uint8_t)buf[3] - 32;
  uint8_t button = code & 0x03;
  if (button != 0 && button != 3)
    return ev;
  ev.type = KeyEvent::KeyType::MOUSE;
  ev.mouse_state = (button == 3) ? KeyEvent::MouseState::RELEASE
                                 : KeyEvent::MouseState::PRESS;
  ev.mouse_x = (uint8_t)buf[4] - 33;
  ev.mouse_y = (uint8_t)buf[5] - 33;
  return ev;
}

KeyEvent IO::parse_escape(const std::string &buf) {
  KeyEvent ev;
  ev.type = KeyEvent::KeyType::SPECIAL;
  bool has_modifier = buf.size() > 3 && buf[3] == ';';
  size_t pos;
  if (!has_modifier) {
    pos = 2;
  } else {
    pos = 5;
    switch (buf.size() > 4 ? buf[4] : 0) {
    case '2':
      ev.modifier = KeyEvent::Modifier::SHIFT;
      break;
    case '3':
      ev.modifier = KeyEvent::Modifier::ALT;
      break;
    case '5':
      ev.modifier = KeyEvent::Modifier::CTRL;
      break;
    case '7':
      ev.modifier = KeyEvent::Modifier::CTRL_ALT;
      break;
    default:
      ev.modifier = KeyEvent::Modifier::NONE;
      break;
    }
  }
  char key = pos < buf.size() ? buf[pos] : 0;
  switch (key) {
  case 'A':
    ev.special_key = KeyEvent::SpecialKey::UP;
    break;
  case 'B':
    ev.special_key = KeyEvent::SpecialKey::DOWN;
    break;
  case 'C':
    ev.special_key = KeyEvent::SpecialKey::RIGHT;
    break;
  case 'D':
    ev.special_key = KeyEvent::SpecialKey::LEFT;
    break;
  case '3':
    ev.special_key = KeyEvent::SpecialKey::DELETE;
    break;
  default:
    ev.special_key = KeyEvent::SpecialKey::UNKNOWN;
    break;
  }
  return ev;
}

KeyEvent IO::read_key() {
  while (true) {
    std::string buf = read_next_unit();
    if (buf.empty()) {
      KeyEvent ev;
      ev.type = KeyEvent::KeyType::NONE;
      return ev;
    }
    if (buf.size() >= 6 && buf[0] == '\x1b' && buf[1] == '[' && buf.compare(2, 4, "200~") == 0) {
      KeyEvent ev;
      std::string pasted;
      if (read_bracketed_paste(pasted)) {
        ev.type = KeyEvent::KeyType::PASTE;
        ev.text = std::move(pasted);
      } else {
        ev.type = KeyEvent::KeyType::NONE;
      }
      return ev;
    }
    if (buf.size() >= 3 && buf[0] == '\x1b' && buf[1] == '[' && buf[2] == 'M') {
      KeyEvent ev = parse_mouse(buf);
      if (ev.type == KeyEvent::KeyType::NONE)
        continue;
      return ev;
    }
    if (buf.size() >= 2 && buf[0] == '\x1b' && buf[1] == '[')
      return parse_escape(buf);
    KeyEvent ev;
    ev.type = KeyEvent::KeyType::CHAR;
    ev.modifier = KeyEvent::Modifier::NONE;
    if (buf.size() == 1) {
      uint8_t c = (uint8_t)buf[0];
      if (c >= 1 && c <= 26 && c != '\t' && c != '\n' && c != '\r' && c != '\x08') {
        ev.modifier = KeyEvent::Modifier::CTRL;
        ev.text = 'a' + c - 1;
        return ev;
      }
    }
    if (buf.size() == 2 && (uint8_t)buf[0] == 0x1B) {
      uint8_t c = (uint8_t)buf[1];
      if (c >= 1 && c <= 26 && c != '\t' && c != '\n' && c != '\r' && c != '\x08') {
        ev.modifier = KeyEvent::Modifier::CTRL_ALT;
        ev.text = 'a' + c - 1;
        return ev;
      }
      ev.modifier = KeyEvent::Modifier::ALT;
      ev.text = buf.substr(1);
      return ev;
    }
    ev.text = std::move(buf);
    return ev;
  }
}
} // namespace bed::internal::io
