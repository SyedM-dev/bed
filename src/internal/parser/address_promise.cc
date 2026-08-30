#include "bed.h"
#include "internal/parser/parser.h"
#include "internal/vase/vase.h"

namespace bed::internal::parser {
buffer::Line AddressPromise::resolve(BEd &ctx) {
  buffer::Line result = std::visit(
    [&](auto const &addr) -> buffer::Line {
      if (!bufname.has_value() || bufname->empty())
        throw ed_error("Buffer name can't be empty.");
      buffer::Line line = {*bufname, 0};
      auto &buf = ctx.buffer(line.buffername);
      using T = std::decay_t<decltype(addr)>;
      if constexpr (std::is_same_v<T, None>) {
        if (line.buffername == ctx.current().buffername)
          line.number = ctx.current().number;
        else
          line.number = buf.lines();
      } else if constexpr (std::is_same_v<T, Current>) {
        if (line.buffername == ctx.current().buffername)
          line.number = ctx.current().number;
        else
          line.number = buf.lines();
      } else if constexpr (std::is_same_v<T, Last>) {
        line.number = buf.lines();
      } else if constexpr (std::is_same_v<T, Number>) {
        line.number = addr.i;
      } else if constexpr (std::is_same_v<T, Mark>) {
        line = ctx.marks.get(addr.m);
        if (line.number == UINT64_MAX)
          throw ed_error("Mark not set.");
      } else if constexpr (std::is_same_v<T, Regex>) {
        if (line.buffername == ctx.current().buffername)
          line.number = ctx.current().number;
        else
          line.number = buf.lines();
        if (buf.lines() > 0 && line.number == 0)
          line.number = 1;
        if (line.number == 0)
          throw ed_error("Can't search empty buffer.");
        std::string_view re = addr.re;
        if (re.size() == 0)
          re = ctx.last_regex;
        if (re.size() == 0)
          throw ed_error("No regex given.");
        if (addr.dir == Direction::Forward)
          line.number = buf.find_next(re, line.number);
        else
          line.number = buf.find_prev(re, line.number);
        ctx.last_regex = re;
      } else if constexpr (std::is_same_v<T, Block>) {
        if (line.buffername == ctx.current().buffername)
          line.number = ctx.current().number;
        else
          line.number = buf.lines();
        if (buf.lines() > 0 && line.number == 0)
          line.number = 1;
        if (addr.dir == Direction::Forward)
          line.number = buf.next_closing(line.number);
        else
          line.number = buf.prev_closing(line.number);
      } else {
        throw ed_error("Unhandled address given.");
      }
      if (offset < 0 && line.number < (uint64_t)-offset)
        throw ed_error("Can't have negative addresses");
      line.number += offset;
      if (line.number > ctx.buffer(line.buffername).lines())
        throw ed_error("Line number too high.");
      return line;
    },
    base
  );
  return result;
}

std::optional<buffer::Line> AddressPromise::get_line(BEd &ctx, std::vector<AddressPromise> &list) {
  std::string bufname = ctx.current().buffername;
  bool prev_given = false;
  bool prev_set = false;
  AddressPromise prev;
  for (std::size_t idx = 0; idx < list.size(); idx++) {
    AddressPromise &curr = list[idx];
    if (curr.bufname.has_value()) {
      if (curr.bufname->empty()) {
        bufname = ctx.current().buffername;
        curr.bufname = bufname;
      } else {
        bufname = *curr.bufname;
      }
    } else {
      curr.bufname = bufname;
    }
    bool is_final = idx + 1 == list.size();
    if (!is_final) {
      if (std::holds_alternative<None>(curr.base)) {
        if (!curr.jumping)
          curr.base = Number(1);
        else
          curr.base = Current();
        curr.offset = 0;
        prev_given = false;
      } else if (std::holds_alternative<LastRange>(curr.base)) {
        curr.bufname = ctx.prev().buffername;
        curr.base = Number(ctx.prev().end);
        prev_given = true;
      } else {
        prev_given = true;
      }
      if (curr.jumping) {
        buffer::Line resolved = curr.resolve(ctx);
        ctx.current() = resolved;
        curr.bufname = resolved.buffername;
        curr.base = Number(resolved.number);
        curr.offset = 0;
      }
      prev = curr;
      prev_set = true;
    } else {
      if (std::holds_alternative<None>(curr.base)) {
        if (prev_set) {
          if (prev_given) {
            curr.base = prev.base;
            curr.offset = prev.offset;
          } else {
            curr.base = Last();
            curr.offset = 0;
          }
        }
      } else if (std::holds_alternative<LastRange>(curr.base)) {
        curr.bufname = ctx.prev().buffername;
        curr.base = Number(ctx.prev().end);
      }
      return curr.resolve(ctx);
    }
  }
  return std::nullopt;
}

std::optional<buffer::Range> AddressPromise::get_range(BEd &ctx, std::vector<AddressPromise> &list) {
  std::string bufname = ctx.current().buffername;
  bool prev_given = false;
  bool prev_set = false;
  AddressPromise prev;
  for (std::size_t idx = 0; idx < list.size(); idx++) {
    AddressPromise &curr = list[idx];
    if (curr.bufname.has_value()) {
      if (curr.bufname->empty()) {
        bufname = ctx.current().buffername;
        curr.bufname = bufname;
      } else {
        bufname = *curr.bufname;
      }
    } else {
      curr.bufname = bufname;
    }
    bool is_final = idx + 1 == list.size();
    if (!is_final) {
      if (std::holds_alternative<None>(curr.base)) {
        if (!curr.jumping)
          curr.base = Number(1);
        else
          curr.base = Current();
        curr.offset = 0;
        prev_given = false;
      } else if (std::holds_alternative<LastRange>(curr.base)) {
        curr.bufname = ctx.prev().buffername;
        curr.base = Number(ctx.prev().end);
        prev_given = true;
      } else {
        prev_given = true;
      }
      if (curr.jumping) {
        buffer::Line resolved = curr.resolve(ctx);
        ctx.current() = resolved;
        curr.bufname = resolved.buffername;
        curr.base = Number(resolved.number);
        curr.offset = 0;
      }
      prev = curr;
      prev_set = true;
    } else {
      if (std::holds_alternative<None>(curr.base)) {
        if (prev_set) {
          if (prev_given) {
            curr.base = prev.base;
            curr.offset = prev.offset;
          } else {
            curr.base = Last();
            curr.offset = 0;
          }
          return buffer::Range(prev.resolve(ctx), curr.resolve(ctx));
        }
      } else if (std::holds_alternative<LastRange>(curr.base)) {
        return ctx.prev();
      }
      if (prev_given)
        return buffer::Range(prev.resolve(ctx), curr.resolve(ctx));
      buffer::Line only = curr.resolve(ctx);
      return buffer::Range(only, only);
    }
  }
  return std::nullopt;
}
} // namespace bed::internal::parser
