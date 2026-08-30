#include "internal/buffer/buffer.h"
#include "bed.h"

namespace bed::internal::buffer {
Buffer::Buffer(std::string name)
    : state(Unmodified), root(nullptr), name(name) {
}

Buffer::~Buffer() {
  vase::Shard::release(root);
}

uint64_t Buffer::lines() {
  if (root)
    return root->lines + 1;
  return 0;
}

uint64_t Buffer::bytes() {
  if (root)
    return root->length + 1;
  return 0;
}

void Buffer::load(BEd &, vase::Shard *text) {
  try {
    vase::Shard::release(root);
    root = text;
    state = buffer::Buffer::Unmodified;
    parser.emplace(root, lines(), syntax::ruby::lang_ruby());
  } catch (...) {
    vase::Shard::release(text);
    throw;
  }
}

void Buffer::append(BEd &ctx, vase::Shard *text, uint64_t line) {
  ctx.prev.buffername = name;
  ctx.prev.start = line + 1;
  ctx.prev.end = line + text->lines + 1;
  root = vase::insert(&ctx.append, root, text, line);
  ctx.marks.insert(name, ctx.prev.start, ctx.prev.end);
  if (parser)
    parser->insert(root, ctx.prev.start, ctx.prev.end);
  state = Modified;
}

void Buffer::remove(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  root = vase::erase(root, start_line, end_line);
  ctx.prev.buffername = name;
  ctx.prev.start = lines() ? 1 : 0;
  ctx.prev.end = lines();
  ctx.marks.erase(name, start_line, end_line - start_line + 1);
  if (parser)
    parser->erase(root, start_line, end_line - start_line + 1);
  state = Modified;
}

void Buffer::join(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  root = vase::join(root, start_line, end_line);
  ctx.prev.buffername = name;
  ctx.prev.start = start_line;
  ctx.prev.end = start_line;
  ctx.marks.collapse(name, start_line, end_line - start_line);
  if (parser)
    parser->erase(root, start_line, end_line - start_line);
  state = Modified;
}

void Buffer::substitute(
  BEd &ctx, uint64_t start_line, uint64_t end_line,
  std::string &regex, std::string &replacement, std::string &options
) {
  // TODO: make substitue return a list of modifications made.
  root = vase::substitute(&ctx.append, root, regex, start_line, end_line, replacement, options);
  /*prev_range.start = start_line;
  prev_range.end = start_line;
  marks.collapse(start_line, end_line - start_line);
  if (parser)
    parser->erase(vase, start_line, end_line - start_line);*/
  state = Modified;
}

vase::Shard *Buffer::copy(uint64_t start_line, uint64_t end_line) {
  return vase::copy(root, start_line, end_line);
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
  ctx.prev.buffername = name;
  ctx.prev.start = start_line;
  ctx.prev.end = end_line;
  if (parser) {
    std::optional<syntax::Parser::Iterator> it_o = parser->get_hl(root, start_line - 1);
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
    vase::Iterator it(root, start_line - 1, Direction::Forward);
    while (it.next() && start_line++ <= end_line)
      std::cout << it.line << std::endl;
  }
}

void Buffer::number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev.buffername = name;
  ctx.prev.start = start_line;
  ctx.prev.end = end_line;
  uint8_t width = 1;
  for (uint64_t n = end_line; n >= 10; n /= 10)
    ++width;
  if (parser) {
    std::optional<syntax::Parser::Iterator> it_o = parser->get_hl(root, start_line - 1);
    auto &it = *it_o;
    while (start_line <= end_line) {
      it.next();
      std::cout << std::setw(width) << start_line << "\t";
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
    vase::Iterator it(root, start_line - 1, Direction::Forward);
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
