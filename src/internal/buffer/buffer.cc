#include "internal/buffer/buffer.h"

namespace bed::internal::buffer {
Buffer::Buffer() : vase("/tmp") {
  line = vase.lines();
  modified = false;
}

Buffer::Buffer(std::string command) : vase(command, "/tmp") {
  line = vase.lines();
  if (!line)
    return;
  prev_range.start = 1;
  prev_range.end = line;
  modified = false;
}

Buffer::Buffer(std::filesystem::path path) : vase(path, "/tmp") {
  line = vase.lines();
  if (!line)
    return;
  prev_range.start = 1;
  prev_range.end = line;
  save_path = path;
  modified = false;
}

void Buffer::load(std::string command) {
  vase::Vase new_vase = vase::Vase(command, "/tmp");
  vase = std::move(new_vase);
  line = vase.lines();
  if (!line) {
    prev_range.start = 0;
    prev_range.end = 0;
  } else {
    prev_range.start = 1;
    prev_range.end = line;
  }
  modified = false;
}

void Buffer::load(std::filesystem::path path) {
  vase::Vase new_vase = vase::Vase(path, "/tmp");
  vase = std::move(new_vase);
  line = vase.lines();
  if (!line) {
    prev_range.start = 0;
    prev_range.end = 0;
  } else {
    prev_range.start = 1;
    prev_range.end = line;
  }
  save_path = path;
  modified = false;
}

void Buffer::jump(uint64_t n_line) {
  if (n_line > vase.lines())
    throw ed_error("Line number too high.");
  line = n_line;
  prev_range.start = line;
  prev_range.end = line;
}

void Buffer::append(std::string text, uint64_t line) {
  using namespace bed::internal::vase;
  Point p = {line, 0};
  if (!vase.lines()) {
    text.pop_back();
  } else if (line == vase.lines()) {
    p.row--;
    p.col = UINT64_MAX;
    text = "\n" + text;
    text.pop_back();
  }
  prev_range.start = p.row + 1;
  vase.insert(&p, text);
  prev_range.end = p.row + 1;
  modified = true;
}

void Buffer::remove(uint64_t start_line, uint64_t end_line) {
  vase.erase({{start_line - 1, 0}, {end_line, 0}});
  prev_range.start = start_line;
  prev_range.end = start_line;
  modified = true;
}

void Buffer::join(uint64_t start_line, uint64_t end_line) {
  vase.regex_search_replace(R"(\n)", {{start_line - 1, 0}, {end_line, 0}}, "", "g");
  prev_range.start = start_line;
  prev_range.end = start_line;
  modified = true;
}

void Buffer::print(uint64_t start_line, uint64_t end_line) {
  vase::Iterator it = vase.iterate(start_line - 1, Direction::Forward);
  while (it.next() && start_line++ <= end_line)
    std::cout << it.line << std::endl;
  prev_range.start = start_line;
  prev_range.end = end_line;
}

std::string Buffer::list_string(std::string_view s) {
  uint32_t width = 80;
  winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col != 0)
    width = ws.ws_col;
  std::string out;
  out.reserve(s.size());
  const uint32_t max_width = width > 1 ? width - 1 : 1;
  uint32_t column = 0;
  auto append = [&](std::string_view text) {
    if (column + text.size() > max_width) {
      out += "\\\n";
      column = 0;
    }
    out += text;
    column += text.size();
  };
  for (unsigned char c : s) {
    switch (c) {
    case '\\':
      append("\\\\");
      break;
    case '$':
      append("\\$");
      break;
    case '\a':
      append("\\a");
      break;
    case '\b':
      append("\\b");
      break;
    case '\f':
      append("\\f");
      break;
    case '\r':
      append("\\r");
      break;
    case '\t':
      append("\\t");
      break;
    case '\v':
      append("\\v");
      break;
    default:
      if (!std::isprint(c)) {
        char buf[5];
        std::snprintf(buf, sizeof(buf), "\\%03o", c);
        append(buf);
      } else {
        append(std::string_view((const char *)&c, 1));
      }
      break;
    }
  }
  out += '$';
  return out;
}
} // namespace bed::internal::buffer
