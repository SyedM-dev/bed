#include "bed.h"
#include "internal/buffer/buffer.h"

namespace bed::internal::buffer {
uint64_t ShardBuffer::lines() {
  if (root)
    return root->lines + 1;
  return 0;
}

uint64_t ShardBuffer::bytes() {
  if (root)
    return root->length + 1;
  return 0;
}

vase::Shard *ShardBuffer::copy(uint64_t start_line, uint64_t end_line) {
  return vase::copy(root, start_line, end_line);
}

uint64_t ShardBuffer::find_next(std::string_view pattern, uint64_t start) {
  return vase::find_next(root, pattern, start);
}

uint64_t ShardBuffer::find_prev(std::string_view pattern, uint64_t start) {
  return vase::find_prev(root, pattern, start);
}

uint64_t ShardBuffer::next_closing(uint64_t start) {
  if (parse.lang) {
    uint64_t closing = syntax::next_closing(parse, start - 1);
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

uint64_t ShardBuffer::prev_closing(uint64_t start) {
  if (parse.lang) {
    return syntax::prev_opening(parse, start - 1) + 1;
  } else {
    if (start > 10)
      return start - 10;
    return 0;
  }
}

void ShardBuffer::print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  if (parse.lang) {
    auto it_o = syntax::get_hl(parse, root, start_line - 1);
    if (!it_o)
      goto h;
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
        ctx.io.apply(token.type);
        ctx.io.write(line.data() + start, end - start);
        ctx.io.reset();
        cursor = end;
      }
      if (cursor < line.size())
        ctx.io.write(line.data() + cursor, line.size() - cursor);
      ctx.io.write_line("");
      ++start_line;
    }
  } else {
  h:
    vase::Iterator it(root, start_line - 1, Direction::Forward);
    while (it.next() && start_line++ <= end_line)
      ctx.io.write_line(it.line);
  }
}

void ShardBuffer::number_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  uint8_t width = 1;
  for (uint64_t n = end_line; n >= 10; n /= 10)
    ++width;
  if (parse.lang) {
    std::optional<syntax::Iterator> it_o = syntax::get_hl(parse, root, start_line - 1);
    if (!it_o)
      goto h;
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
        ctx.io.apply(token.type);
        ctx.io.write(line.data() + start, end - start);
        ctx.io.reset();
        cursor = end;
      }
      if (cursor < line.size())
        ctx.io.write(line.data() + cursor, line.size() - cursor);
      ctx.io.write_line("");
      ++start_line;
    }
  } else {
  h:
    vase::Iterator it(root, start_line - 1, Direction::Forward);
    while (it.next() && start_line <= end_line)
      ctx.io.write_line(std::format("{:>{}}\t{}", start_line++, width, it.line));
  }
}

void ShardBuffer::list_print(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  vase::Iterator it(root, start_line - 1, Direction::Forward);
  while (it.next() && start_line++ <= end_line)
    ctx.io.write_line(list_string(it.line));
}
} // namespace bed::internal::buffer
