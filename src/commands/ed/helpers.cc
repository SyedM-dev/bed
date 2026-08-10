#include "commands/ed/ed.h"

namespace crib::commands::ed {
void Ed::append(std::string text, uint64_t line) {
  using namespace crib::internal::vase;
  Point p = {line, 0};
  if (!vase.lines()) {
    text.pop_back();
  } else if (line == vase.lines()) {
    p.row--;
    p.col = UINT64_MAX;
    text = "\n" + text;
    text.pop_back();
  }
  vase.insert(&p, text);
}

void Ed::remove(uint64_t start_line, uint64_t end_line) {
  vase.erase({{start_line - 1, 0}, {end_line, 0}});
}

void Ed::join(uint64_t start_line, uint64_t end_line) {
  vase.regex_search_replace("\\n", {{start_line - 1, 0}, {end_line, 0}}, "", "");
}

std::string Ed::list_string(std::string_view s) {
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
} // namespace crib::commands::ed
