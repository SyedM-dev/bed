#include "internal/commands/commands.h"
#include "bed.h"
#include "internal/commands/suffixes.h"

namespace bed::internal::commands {
void Suffix::register_suffixes(BEd &ctx) {
  ctx.suffixes['p' - 'a'] = Suffix{
    .desc = "Prints current line.",
    .handle = [](BEd &ctx) {
      auto line = ctx.active->line;
      ctx.active->print(ctx, line, line);
      ctx.active->jump(line);
    }
  };
  ctx.suffixes['n' - 'a'] = Suffix{
    .desc = "Prints current line with line number.",
    .handle = [](BEd &ctx) {
      auto line = ctx.active->line;
      ctx.active->number_print(ctx, line, line);
      ctx.active->jump(line);
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
      ctx.active->print(ctx, line, line);
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
      throw fatal_error("Quitting", 0);
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
    "j",
    Command{
      .address_mode = Command::AddressMode::Range,
      .suffix = Command::SuffixKind::Suffix,
      .desc = "Join a set of lines (default: .,.+1)",
      .accept_zero = true,
      .handle = [](BEd &ctx, std::span<const uint64_t> addresses, std::string_view) {
        std::cout << addresses.size() << "\n";
        uint64_t start_line = ctx.active->line;
        uint64_t end_line = ctx.active->line + 1;
        if (addresses.size() == 1)
          return;
        else if (addresses.size() == 2)
          start_line = addresses[0], end_line = addresses[1];
        ctx.active->join(start_line, end_line);
        ctx.active->jump(start_line);
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
        throw fatal_error("Quitting", 0);
      }
    }
  );
  ctx.commands.insert(
    "Q",
    Command{
      .address_mode = Command::AddressMode::None,
      .suffix = Command::SuffixKind::None,
      .desc = "Force quitting.",
      .accept_zero = false,
      .handle = [](BEd &, std::span<const uint64_t>, std::string_view) {
        throw fatal_error("Force Quitting", 0);
      }
    }
  );
  ctx.commands.insert(
    "p",
    Command{
      .address_mode = Command::AddressMode::Range,
      .suffix = Command::SuffixKind::Suffix,
      .desc = "Print range (default .,.)",
      .accept_zero = false,
      .handle = [](BEd &ctx, std::span<const uint64_t> addresses, std::string_view) {
        uint64_t start_line = ctx.active->line;
        uint64_t end_line = ctx.active->line;
        if (addresses.size() == 1)
          start_line = addresses[0], end_line = addresses[0];
        else if (addresses.size() == 2)
          start_line = addresses[0], end_line = addresses[1];
        ctx.active->print(ctx, start_line, end_line);
        ctx.active->jump(end_line);
      }
    }
  );
  ctx.commands.insert(
    "n",
    Command{
      .address_mode = Command::AddressMode::Range,
      .suffix = Command::SuffixKind::Suffix,
      .desc = "Print range with line numbers (default .,.)",
      .accept_zero = false,
      .handle = [](BEd &ctx, std::span<const uint64_t> addresses, std::string_view) {
        uint64_t start_line = ctx.active->line;
        uint64_t end_line = ctx.active->line;
        if (addresses.size() == 1)
          start_line = addresses[0], end_line = addresses[0];
        else if (addresses.size() == 2)
          start_line = addresses[0], end_line = addresses[1];
        ctx.active->number_print(ctx, start_line, end_line);
        ctx.active->jump(end_line);
      }
    }
  );
  ctx.commands.insert(
    "=",
    Command{
      .address_mode = Command::AddressMode::Range,
      .suffix = Command::SuffixKind::Suffix,
      .desc = "Print line number(s)",
      .accept_zero = true,
      .handle = [](BEd &ctx, std::span<const uint64_t> addresses, std::string_view) {
        if (!addresses.size())
          std::cout << ctx.active->vase.lines() << std::endl;
        else if (addresses.size() == 1)
          std::cout << addresses[0] << std::endl;
        else
          std::cout << addresses[0] << "," << addresses[1] << std::endl;
      }
    }
  );
  ctx.commands.insert(
    "k",
    Command{
      .address_mode = Command::AddressMode::Single,
      .suffix = Command::SuffixKind::Continuation,
      .desc = "Mark a line.",
      .accept_zero = true,
      .handle = [](BEd &ctx, std::span<const uint64_t> addresses, std::string_view args) {
        if (args.size() < 1 || args.size() > 2)
          throw ed_error("Malformed mark command");
        if (addresses.size())
          ctx.active->marks.set(args[0], addresses[0]);
        else
          ctx.active->marks.set(args[0], ctx.active->line);
        if (args.size() > 1)
          ctx.suffix_handle(args[1]);
      }
    }
  );
  ctx.commands.insert(
    "debug",
    Command{
      .address_mode = Command::AddressMode::None,
      .suffix = Command::SuffixKind::None,
      .desc = "",
      .accept_zero = false,
      .handle = [](BEd &ctx, std::span<const uint64_t>, std::string_view) {
        syntax::dump_events(ctx.active->parser->root);
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
