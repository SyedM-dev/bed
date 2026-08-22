#include "internal/buffer/buffer.h"
#include "bed.h"

namespace bed::internal::buffer {
Buffer::Buffer() : vase("/tmp") {
  parser.emplace(vase, vase.lines(), syntax::ruby::lang_ruby());
  line = vase.lines();
  modified = false;
}

Buffer::Buffer(std::string command) : vase(command, "/tmp") {
  parser.emplace(vase, vase.lines(), syntax::ruby::lang_ruby());
  line = vase.lines();
  if (!line)
    return;
  prev_range.start = 1;
  prev_range.end = line;
  modified = false;
}

Buffer::Buffer(std::filesystem::path path) : vase(path, "/tmp") {
  parser.emplace(vase, vase.lines(), syntax::ruby::lang_ruby());
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
  parser.emplace(vase, vase.lines(), syntax::ruby::lang_ruby());
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
  parser.emplace(vase, vase.lines(), syntax::ruby::lang_ruby());
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
  marks.insert(prev_range.start, prev_range.end);
  if (parser)
    parser->insert(vase, prev_range.start, prev_range.end);
  modified = true;
}

void Buffer::remove(uint64_t start_line, uint64_t end_line) {
  vase.erase({{start_line - 1, 0}, {end_line, 0}});
  prev_range.start = start_line;
  prev_range.end = start_line;
  marks.erase(start_line, end_line - start_line + 1);
  if (parser)
    parser->erase(vase, start_line, end_line - start_line + 1);
  modified = true;
}

void Buffer::join(uint64_t start_line, uint64_t end_line) {
  vase.regex_search_replace(R"(\n)", {{start_line - 1, 0}, {end_line, 0}}, "", "g");
  prev_range.start = start_line;
  prev_range.end = start_line;
  marks.collapse(start_line, end_line - start_line);
  if (parser)
    parser->erase(vase, start_line, end_line - start_line);
  modified = true;
}

inline void apply(std::ostream &out, const Highlight &hl) {
  out << "\x1b[0m";
  const uint8_t r = (hl.fg >> 16) & 0xff;
  const uint8_t g = (hl.fg >> 8) & 0xff;
  const uint8_t b = hl.fg & 0xff;
  out << "\x1b[38;2;"
      << (unsigned)r << ';'
      << (unsigned)g << ';'
      << (unsigned)b << 'm';
  if (hl.bg != 0) {
    const uint8_t br = (hl.bg >> 16) & 0xff;
    const uint8_t bg = (hl.bg >> 8) & 0xff;
    const uint8_t bb = hl.bg & 0xff;
    out << "\x1b[48;2;"
        << (unsigned)br << ';'
        << (unsigned)bg << ';'
        << (unsigned)bb << 'm';
  }
  if (hl.flags & Highlight::Bold)
    out << "\x1b[1m";
  if (hl.flags & Highlight::Italic)
    out << "\x1b[3m";
  if (hl.flags & Highlight::Underline)
    out << "\x1b[4m";
  if (hl.flags & Highlight::Strikethrough)
    out << "\x1b[9m";
}

inline void reset(std::ostream &out) {
  out << "\x1b[0m";
}

void Buffer::print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  prev_range.start = start_line;
  prev_range.end = end_line;
  if (parser) {
    std::optional<syntax::Parser::Iterator> it_o = parser->get_hl(vase, start_line - 1);
    auto &it = *it_o;
    while (start_line <= end_line) {
      it.next();
      const std::string &line = it.it->line;
      const auto &tokens = it.tokens;
      uint32_t cursor = 0;
      for (const auto &token : tokens) {
        const uint32_t start = token.start;
        const uint32_t end = token.end;
        if (start > line.size())
          break;
        if (end > line.size())
          break;
        if (cursor < start) {
          std::cout.write(
            line.data() + cursor,
            start - cursor
          );
        }
        const auto highlight = ctx.theme.get(token);
        apply(std::cout, highlight);
        std::cout.write(line.data() + start, end - start);
        reset(std::cout);
        cursor = end;
      }
      if (cursor < line.size())
        std::cout.write(line.data() + cursor, line.size() - cursor);
      std::cout << std::endl;
      ++start_line;
    }
  } else {
    vase::Iterator it = vase.iterate(start_line - 1, Direction::Forward);
    while (it.next() && start_line++ <= end_line)
      std::cout << it.line << std::endl;
  }
}

void Buffer::number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  prev_range.start = start_line;
  prev_range.end = end_line;
  uint8_t width = 1;
  for (uint64_t n = end_line; n >= 10; n /= 10)
    ++width;
  if (parser) {
    std::optional<syntax::Parser::Iterator> it_o = parser->get_hl(vase, start_line - 1);
    auto &it = *it_o;
    while (start_line <= end_line) {
      it.next();
      std::cout << std::setw(width) << it.at << "\t";
      const std::string &line = it.it->line;
      const auto &tokens = it.tokens;
      uint32_t cursor = 0;
      for (const auto &token : tokens) {
        const uint32_t start = token.start;
        const uint32_t end = token.end;
        if (start > line.size())
          break;
        if (end > line.size())
          break;
        if (cursor < start) {
          std::cout.write(
            line.data() + cursor,
            start - cursor
          );
        }
        const auto highlight = ctx.theme.get(token);
        apply(std::cout, highlight);
        std::cout.write(line.data() + start, end - start);
        reset(std::cout);
        cursor = end;
      }
      if (cursor < line.size())
        std::cout.write(line.data() + cursor, line.size() - cursor);
      std::cout << std::endl;
      ++start_line;
    }
  } else {
    vase::Iterator it = vase.iterate(start_line - 1, Direction::Forward);
    while (it.next() && start_line <= end_line)
      std::cout << std::setw(width) << start_line++ << "\t" << it.line << std::endl;
  }
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
