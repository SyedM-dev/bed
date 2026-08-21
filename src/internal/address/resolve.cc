#include "bed.h"
#include "internal/address/address.h"

namespace bed::internal::address {
uint64_t Address::resolve(BEd &ctx) {
  uint64_t result = std::visit(
    [&](auto const &addr) -> uint64_t {
      uint64_t line = 0;
      using T = std::decay_t<decltype(addr)>;
      if constexpr (std::is_same_v<T, std::monostate>) {
        throw address_error("empty address");
      } else if constexpr (std::is_same_v<T, None>) {
        throw address_error("no address");
      } else if constexpr (std::is_same_v<T, Current>) {
        line = ctx.active->line;
      } else if constexpr (std::is_same_v<T, Last>) {
        line = ctx.active->vase.lines();
      } else if constexpr (std::is_same_v<T, Number>) {
        line = addr.i;
      } else if constexpr (std::is_same_v<T, Block>) {
        uint64_t current_line = ctx.active->line;
        if (ctx.active->vase.lines() > 0 && current_line == 0)
          current_line = 1;
        if (addr.dir == Direction::Forward) {
          if (ctx.active->parser) {
            uint64_t closing = ctx.active->parser->next_closing(current_line - 1);
            if (closing == UINT64_MAX)
              line = ctx.active->vase.lines();
            else
              line = closing + 1;
          } else {
            line = current_line + 10;
            if (line > ctx.active->vase.lines())
              line = ctx.active->vase.lines();
          }
        } else {
          if (ctx.active->parser) {
            line = ctx.active->parser->prev_opening(current_line - 1) + 1;
          } else {
            if (current_line > 10)
              line = current_line - 10;
            else
              line = 0;
          }
        }
      } else {
        throw ed_error("Unhandled address given.");
      }
      if (offset < 0 && line < (uint64_t)-offset)
        throw address_error("Can't have negative addresses");
      line += offset;
      if (line > ctx.active->vase.lines())
        throw address_error("Line number too high.");
      return line;
    },
    base
  );

  return result + offset;
}
}; // namespace bed::internal::address
