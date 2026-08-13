#include "bed.h"
#include "internal/address/address.h"

namespace bed::internal::address {
Address::Result Address::handle(BEd &ctx, std::string &cmd, uint64_t &i) {
  bool prev_given = false;
  Address prev;
  Address curr;
  while (i < cmd.size()) {
    if (cmd[i] == '%') {
      prev_given = true;
      prev.base = Number(ctx.active->prev_range.start);
      prev.offset = 0;
      curr.base = Number(ctx.active->prev_range.end);
      curr.offset = 0;
    } else {
      curr = Address(cmd, i);
    }
    if (i < cmd.size() && (cmd[i] == ',' || cmd[i] == ';')) {
      if (std::holds_alternative<None>(curr.base)) {
        if (cmd[i] == ',')
          curr.base = Number(1);
        else
          curr.base = Current();
        curr.offset = 0;
        prev_given = false;
      } else {
        if (cmd[i] == ';')
          ctx.active->jump(curr.resolve(ctx));
        prev_given = true;
      }
      prev = std::move(curr);
    } else {
      if (std::holds_alternative<None>(curr.base)) {
        if (std::holds_alternative<std::monostate>(prev.base))
          return {};
        if (prev_given) {
          curr = prev;
        } else {
          curr.base = Last();
          curr.offset = 0;
        }
        return {{prev.resolve(ctx), curr.resolve(ctx)}, 2};
      }
      return {{curr.resolve(ctx)}, 1};
    }
  }
  return {};
}
}; // namespace bed::internal::address
