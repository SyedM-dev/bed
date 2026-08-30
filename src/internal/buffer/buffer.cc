#include "internal/buffer/buffer.h"
#include "bed.h"

namespace bed::internal::buffer {
GenericBuffer::~GenericBuffer() {
  vase::Shard::release(root);
}

bool GenericBuffer::waste() {
  return save_path.empty()
         && root == nullptr;
}

uint64_t GenericBuffer::lines() {
  if (root)
    return root->lines + 1;
  return 0;
}

uint64_t GenericBuffer::bytes() {
  if (root)
    return root->length + 1;
  return 0;
}

void GenericBuffer::load(BEd &ctx, vase::Shard *text) {
  try {
    if (lines())
      ctx.marks.erase(name, 1, lines());
    vase::Shard::release(root);
    root = text;
    state = buffer::GenericBuffer::Unmodified;
    if (!text) {
      ctx.prev().buffername = name;
      ctx.prev().start = 0;
      ctx.prev().end = 0;
    } else {
      ctx.prev().buffername = name;
      ctx.prev().start = 1;
      ctx.prev().end = text->lines + 1;
    }
    parser.emplace(root, lines(), syntax::ruby::lang_ruby());
  } catch (...) {
    vase::Shard::release(text);
    throw;
  }
}

void GenericBuffer::load(BEd &ctx) {
  if (save_path.empty())
    throw ed_error("File cannot be implied.");
  load(ctx, save_path);
}

void GenericBuffer::load(BEd &ctx, const char *cmd) {
  auto text = vase::Shard::from_command(cmd, true);
  try {
    if (lines())
      ctx.marks.erase(name, 1, lines());
    vase::Shard::release(root);
    root = text;
    state = buffer::GenericBuffer::Unmodified;
    if (!text) {
      ctx.prev().buffername = name;
      ctx.prev().start = 0;
      ctx.prev().end = 0;
    } else {
      ctx.prev().buffername = name;
      ctx.prev().start = 1;
      ctx.prev().end = text->lines + 1;
    }
    parser.emplace(root, lines(), syntax::ruby::lang_ruby());
  } catch (...) {
    vase::Shard::release(text);
    throw;
  }
}

void GenericBuffer::load(BEd &ctx, std::filesystem::path path) {
  auto text = vase::Shard::from_file(path, true);
  try {
    if (lines())
      ctx.marks.erase(name, 1, lines());
    vase::Shard::release(root);
    root = text;
    state = buffer::GenericBuffer::Unmodified;
    if (!text) {
      ctx.prev().buffername = name;
      ctx.prev().start = 0;
      ctx.prev().end = 0;
    } else {
      ctx.prev().buffername = name;
      ctx.prev().start = 1;
      ctx.prev().end = text->lines + 1;
    }
    parser.emplace(root, lines(), syntax::ruby::lang_ruby());
  } catch (...) {
    vase::Shard::release(text);
    throw;
  }
}

void GenericBuffer::append(BEd &ctx, vase::Shard *text, uint64_t line) {
  ctx.prev().buffername = name;
  ctx.prev().start = line + 1;
  ctx.prev().end = line + text->lines + 1;
  root = vase::insert(&ctx.append, root, text, line);
  ctx.marks.insert(name, ctx.prev().start, ctx.prev().end);
  if (parser)
    parser->insert(root, ctx.prev().start, ctx.prev().end);
  state = Modified;
}

void GenericBuffer::remove(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  root = vase::erase(root, start_line, end_line);
  ctx.prev().buffername = name;
  ctx.prev().start = lines() ? 1 : 0;
  ctx.prev().end = lines();
  ctx.marks.erase(name, start_line, end_line - start_line + 1);
  if (parser)
    parser->erase(root, start_line, end_line - start_line + 1);
  state = Modified;
}

void GenericBuffer::join(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  root = vase::join(root, start_line, end_line);
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = start_line;
  ctx.marks.collapse(name, start_line, end_line - start_line);
  if (parser)
    parser->erase(root, start_line, end_line - start_line);
  state = Modified;
}

void GenericBuffer::substitute(
  BEd &ctx, uint64_t start_line, uint64_t end_line,
  std::string &regex, std::string &replacement, std::string &options
) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  if (parser)
    parser->begin_edit();
  root = vase::substitute(
    &ctx.append,
    root,
    regex,
    start_line,
    end_line,
    replacement,
    options,
    [&](uint64_t line, uint64_t old_lines, uint64_t new_lines) {
      if (old_lines) {
        ctx.marks.erase(name, line, old_lines);
        if (parser)
          parser->erase(line, old_lines);
      }
      if (new_lines) {
        ctx.marks.insert(name, line, line + new_lines - 1);
        if (parser)
          parser->insert(line, line + new_lines - 1);
      }
    }
  );
  if (parser)
    parser->end_edit(root);
  ctx.prev().buffername = name;
  state = Modified;
}

vase::Shard *GenericBuffer::copy(uint64_t start_line, uint64_t end_line) {
  return vase::copy(root, start_line, end_line);
}

uint64_t GenericBuffer::find_next(std::string_view pattern, uint64_t start) {
  return vase::find_next(root, pattern, start);
}

uint64_t GenericBuffer::find_prev(std::string_view pattern, uint64_t start) {
  return vase::find_prev(root, pattern, start);
}

uint64_t GenericBuffer::next_closing(uint64_t start) {
  if (parser.has_value()) {
    uint64_t closing = parser->next_closing(start - 1);
    if (closing == UINT64_MAX)
      return lines();
    return closing + 1;
  } else {
    start += 10;
    if (start > lines())
      return lines();
    return start;
  }
}

uint64_t GenericBuffer::prev_closing(uint64_t start) {
  if (parser.has_value()) {
    return parser->prev_opening(start - 1) + 1;
  } else {
    if (start > 10)
      return start - 10;
    return 0;
  }
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

void GenericBuffer::print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
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

void GenericBuffer::number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
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

/*std::string GenericBuffer::list_string(std::string_view s) {
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
}*/
} // namespace bed::internal::buffer
