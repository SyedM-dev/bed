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
        if (addr.dir == Direction::Forward) {
          if (ctx.active->parser) {
            if (ctx.active->line == 0)
              ctx.active->line = 1;
            line = ctx.active->parser->next_closing(ctx.active->line - 1) + 1;
            if (line > ctx.active->vase.lines())
              line = ctx.active->vase.lines();
          } else {
            line = ctx.active->line + 10;
          }
        } else {
          if (ctx.active->parser) {
            if (ctx.active->line == 0)
              ctx.active->line = 1;
            line = ctx.active->parser->prev_opening(ctx.active->line - 1) + 1;
          } else {
            line = ctx.active->line - 10;
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
