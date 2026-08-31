#include "bed.h"
#include "internal/buffer/buffer.h"

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
  if (lines())
    ctx.marks.erase(name, 1, lines());
  vase::Shard::release(root);
  vase::Shard::retain(text);
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
}

void GenericBuffer::set_filename(std::filesystem::path path) {
  save_path = path;
}

std::filesystem::path GenericBuffer::filename() {
  return save_path;
};

void GenericBuffer::append(BEd &ctx, vase::Shard *text, uint64_t line) {
  if (!text)
    return;
  ctx.prev().buffername = name;
  ctx.prev().start = line + 1;
  ctx.prev().end = line + text->lines + 1;
  root = vase::insert(&ctx.append, root, text, line);
  ctx.marks.insert(name, line, text->lines + 1);
  if (parser)
    parser->insert(root, line, text->lines + 1);
  state = Modified;
}

void GenericBuffer::remove(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  root = vase::erase(root, start_line, end_line);
  ctx.prev().buffername = name;
  ctx.prev().start = std::min(start_line, lines());
  ctx.prev().end = std::min(start_line, lines());
  ctx.marks.erase(name, start_line, end_line - start_line + 1);
  if (parser)
    parser->erase(root, start_line, end_line - start_line + 1);
  state = Modified;
}

void GenericBuffer::replace(BEd &ctx, vase::Shard *text, uint64_t start_line, uint64_t end_line) {
  if (!text) {
    remove(ctx, start_line, end_line);
    return;
  }
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = start_line + text->lines;
  uint64_t new_count = text->lines + 1;
  uint64_t old_count = end_line - start_line + 1;
  root = vase::replace(root, text, start_line, end_line);
  if (parser) {
    parser->begin_edit();
    parser->erase(start_line, old_count);
    parser->insert(start_line, new_count);
    parser->end_edit(root);
  }
  if (new_count > old_count) {
    uint64_t diff = new_count - old_count;
    ctx.marks.insert(name, end_line, diff);
  } else if (new_count < old_count) {
    uint64_t diff = old_count - new_count;
    ctx.marks.collapse(name, start_line + new_count - 1, diff);
  }
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
        ctx.marks.insert(name, line, line + new_lines);
        if (parser)
          parser->insert(line, new_lines);
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

inline void apply(io::IO &io, const Highlight &hl) {
  io.write("\x1b[0m");
  const uint8_t r = (hl.fg >> 16) & 0xff;
  const uint8_t g = (hl.fg >> 8) & 0xff;
  const uint8_t b = hl.fg & 0xff;
  io.write(std::format("\x1b[38;2;{};{};{}m", r, g, b));
  if (hl.bg != 0) {
    const uint8_t br = (hl.bg >> 16) & 0xff;
    const uint8_t bg = (hl.bg >> 8) & 0xff;
    const uint8_t bb = hl.bg & 0xff;
    io.write(std::format("\x1b[48;2;{};{};{}m", br, bg, bb));
  }
  if (hl.flags & Highlight::Bold)
    io.write("\x1b[1m");
  if (hl.flags & Highlight::Italic)
    io.write("\x1b[3m");
  if (hl.flags & Highlight::Underline)
    io.write("\x1b[4m");
  if (hl.flags & Highlight::Strikethrough)
    io.write("\x1b[9m");
}

inline void reset(io::IO &io) {
  io.write("\x1b[0m");
}

void GenericBuffer::print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  if (parser) {
    auto it_o = parser->get_hl(root, start_line - 1);
    auto &it = *it_o;
    while (start_line <= end_line) {
      it.next();
      const std::string &line = it.it->line;
      const auto &tokens = it.tokens;
      uint32_t cursor = 0;
      for (const auto &token : tokens) {
        const uint32_t start = token.start;
        const uint32_t end = token.end;
        if (start > line.size() || end > line.size())
          break;
        if (cursor < start)
          ctx.io.write(line.data() + cursor, start - cursor);
        const auto highlight = ctx.theme.get(token);
        apply(ctx.io, highlight);
        ctx.io.write(line.data() + start, end - start);
        reset(ctx.io);
        cursor = end;
      }
      if (cursor < line.size())
        ctx.io.write(line.data() + cursor, line.size() - cursor);
      ctx.io.write_line("");
      ++start_line;
    }
  } else {
    vase::Iterator it(root, start_line - 1, Direction::Forward);
    while (it.next() && start_line++ <= end_line)
      ctx.io.write_line(it.line);
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
      ctx.io.write(std::format("{:>{}}\t", start_line, width));
      const std::string &line = it.it->line;
      const auto &tokens = it.tokens;
      uint32_t cursor = 0;
      for (const auto &token : tokens) {
        const uint32_t start = token.start;
        const uint32_t end = token.end;
        if (start > line.size() || end > line.size())
          break;
        if (cursor < start)
          ctx.io.write(line.data() + cursor, start - cursor);
        const auto highlight = ctx.theme.get(token);
        apply(ctx.io, highlight);
        ctx.io.write(line.data() + start, end - start);
        reset(ctx.io);
        cursor = end;
      }
      if (cursor < line.size())
        ctx.io.write(line.data() + cursor, line.size() - cursor);
      ctx.io.write_line("");
      ++start_line;
    }
  } else {
    vase::Iterator it(root, start_line - 1, Direction::Forward);
    while (it.next() && start_line <= end_line)
      ctx.io.write_line(std::format("{:>{}}\t{}", start_line++, width, it.line));
  }
}

void GenericBuffer::list_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  vase::Iterator it(root, start_line - 1, Direction::Forward);
  while (it.next() && start_line++ <= end_line)
    ctx.io.write_line(list_string(it.line));
}
} // namespace bed::internal::buffer
