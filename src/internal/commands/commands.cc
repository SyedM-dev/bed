#include "internal/commands/commands.h"
#include "bed.h"
#include "internal/commands/suffixes.h"

namespace bed::internal::commands {
void Suffix::register_suffixes(BEd &ctx) {
  ctx.suffixes['p' - 'a'] = Suffix{
    .desc = "Prints current line.",
    .handle = [](BEd &ctx) {
      auto line = ctx.active->line;
      ctx.active->print(line, line);
    }
  };
}

void Command::register_posix(BEd &ctx) {
  ctx.no_op = Command{
    .address_mode = Command::AddressMode::Single,
    .suffix = Command::SuffixKind::None,
    .desc = "Prints a line and jumps to it (default: .+1)",
    .accept_zero = false,
    .handle = [](BEd &ctx, std::span<const uint64_t> addresses, std::string_view) {
      uint64_t line;
      if (addresses.size())
        line = addresses[0];
      else
        line = ctx.active->line + 1;
      if (line == 0)
        throw ed_error("Line 0 is invalid.");
      ctx.active->jump(line);
      ctx.active->print(line, line);
    }
  };
  ctx.eof_op = Command{
    .address_mode = Command::AddressMode::None,
    .suffix = Command::SuffixKind::None,
    .desc = "Try quitting.",
    .accept_zero = false,
    .handle = [](BEd &ctx, std::span<const uint64_t>, std::string_view) {
      for (auto &[name, buffer] : ctx.buffers)
        if (buffer->modified)
          throw ed_error("Buffer " + name + " modified.");
      throw fatal_error("", 0);
    }
  };
  ctx.commands.insert(
    "a",
    Command{
      .address_mode = Command::AddressMode::Single,
      .suffix = Command::SuffixKind::Suffix,
      .desc = "Append lines at address (default: .)",
      .accept_zero = true,
      .handle = [](BEd &, std::span<const uint64_t>, std::string_view) {
        // TODO: start a text editing session, then append its stuff.
      }
    }
  );
  ctx.commands.insert(
    "q",
    Command{
      .address_mode = Command::AddressMode::None,
      .suffix = Command::SuffixKind::None,
      .desc = "Try quitting.",
      .accept_zero = false,
      .handle = [](BEd &ctx, std::span<const uint64_t>, std::string_view) {
        for (auto &[name, buffer] : ctx.buffers)
          if (buffer->modified)
            throw ed_error("Buffer " + name + " modified.");
        throw fatal_error("", 0);
      }
    }
  );
  ctx.commands.insert(
    "E",
    Command{
      .address_mode = Command::AddressMode::None,
      .suffix = Command::SuffixKind::Argument,
      .desc = "Load a file into the current buffer.",
      .accept_zero = true,
      .handle = [](BEd &ctx, std::span<const uint64_t>, std::string_view file) {
        bool empty = false;
        if (file.empty())
          empty = true;
        uint64_t i = 0;
        while (i < file.length() && (file[i] == ' ' || file[i] == '\t'))
          i++;
        if (i >= file.size())
          empty = true;
        if (empty) {
          if (ctx.active->save_path == "")
            throw ed_error("Need filename!");
          ctx.active->load(ctx.active->save_path);
        } else if (file[i] == '!') {
          file = file.substr(1);
          ctx.active->load(file);
        } else {
          ctx.active->load(std::filesystem::path(file));
        }
        std::cout << ctx.active->vase.length() << std::endl;
      }
    }
  );
}
} // namespace bed::internal::commands
