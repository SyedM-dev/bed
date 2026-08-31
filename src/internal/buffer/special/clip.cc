#include "bed.h"
#include "internal/buffer/buffer.h"

namespace bed::internal::buffer {
ClipBuffer::~ClipBuffer() {}

bool ClipBuffer::waste() {
  return false;
}

uint64_t ClipBuffer::lines() {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  uint64_t lines = s ? s->lines + 1 : 0;
  vase::Shard::release(s);
  return lines;
}

uint64_t ClipBuffer::bytes() {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  uint64_t length = s ? s->length + 1 : 0;
  vase::Shard::release(s);
  return length;
}

void ClipBuffer::clip_write(vase::Shard *text) {
  FILE *pipe = popen("xclip -selection clipboard -i", "w");
  if (!pipe)
    throw ed_error("can't access clipboard");
  auto s = vase::to_string(text);
  fwrite(s.data(), 1, s.length(), pipe);
  if (pclose(pipe) != 0)
    throw ed_error("can't write clipboard");
}

void ClipBuffer::load(BEd &ctx, vase::Shard *text) {
  clip_write(text);
  if (!text) {
    ctx.prev().buffername = name;
    ctx.prev().start = 0;
    ctx.prev().end = 0;
  } else {
    ctx.prev().buffername = name;
    ctx.prev().start = 1;
    ctx.prev().end = text->lines + 1;
  }
}

void ClipBuffer::set_filename(std::filesystem::path) {}

std::filesystem::path ClipBuffer::filename() {
  return "";
};

void ClipBuffer::append(BEd &ctx, vase::Shard *text, uint64_t line) {
  ctx.prev().buffername = name;
  ctx.prev().start = line + 1;
  ctx.prev().end = line + text->lines + 1;
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  s = vase::insert(&ctx.append, s, text, line);
  clip_write(s);
  vase::Shard::release(s);
  ctx.marks.insert(name, ctx.prev().start, ctx.prev().end);
}

void ClipBuffer::remove(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  s = vase::erase(s, start_line, end_line);
  clip_write(s);
  ctx.prev().buffername = name;
  ctx.prev().start = std::min(start_line, s ? s->lines + 1 : 0);
  ctx.prev().end = std::min(start_line, s ? s->lines + 1 : 0);
  vase::Shard::release(s);
  ctx.marks.erase(name, start_line, end_line - start_line + 1);
}

void ClipBuffer::join(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  s = vase::join(s, start_line, end_line);
  clip_write(s);
  vase::Shard::release(s);
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = start_line;
  ctx.marks.collapse(name, start_line, end_line - start_line);
}

void ClipBuffer::substitute(
  BEd &ctx, uint64_t start_line, uint64_t end_line,
  std::string &regex, std::string &replacement, std::string &options
) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  s = vase::substitute(
    &ctx.append,
    s,
    regex,
    start_line,
    end_line,
    replacement,
    options,
    [&](uint64_t line, uint64_t old_lines, uint64_t new_lines) {
      if (old_lines)
        ctx.marks.erase(name, line, old_lines);
      if (new_lines)
        ctx.marks.insert(name, line, line + new_lines - 1);
    }
  );
  clip_write(s);
  vase::Shard::release(s);
  ctx.prev().buffername = name;
}

vase::Shard *ClipBuffer::copy(uint64_t start_line, uint64_t end_line) {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  vase::Shard *o = vase::copy(s, start_line, end_line);
  vase::Shard::release(s);
  return o;
}

uint64_t ClipBuffer::find_next(std::string_view pattern, uint64_t start) {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  uint64_t line = vase::find_next(s, pattern, start);
  vase::Shard::release(s);
  return line;
}

uint64_t ClipBuffer::find_prev(std::string_view pattern, uint64_t start) {
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  uint64_t line = vase::find_prev(s, pattern, start);
  vase::Shard::release(s);
  return line;
}

uint64_t ClipBuffer::next_closing(uint64_t start) {
  start += 10;
  uint64_t line = lines();
  if (start > line)
    return line;
  return start;
}

uint64_t ClipBuffer::prev_closing(uint64_t start) {
  if (start > 10)
    return start - 10;
  return 0;
}

void ClipBuffer::print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  vase::Iterator it(s, start_line - 1, Direction::Forward);
  while (it.next() && start_line++ <= end_line)
    ctx.io.write_line(it.line);
  vase::Shard::release(s);
}

void ClipBuffer::number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  uint8_t width = 1;
  for (uint64_t n = end_line; n >= 10; n /= 10)
    ++width;
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  vase::Iterator it(s, start_line - 1, Direction::Forward);
  while (it.next() && start_line <= end_line)
    ctx.io.write_line(std::format("{:>{}}\t{}", start_line++, width, it.line));
  vase::Shard::release(s);
}

void ClipBuffer::list_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  auto s = vase::Shard::from_command("xclip -selection clipboard -o", true);
  vase::Iterator it(s, start_line - 1, Direction::Forward);
  while (it.next() && start_line++ <= end_line)
    ctx.io.write_line(list_string(it.line));
  vase::Shard::release(s);
}
} // namespace bed::internal::buffer
