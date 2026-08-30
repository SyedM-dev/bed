#include "internal/functions/functions.h"
#include "bed.h"
#include "internal/functions/suffixes.h"

namespace bed::internal::functions {
void Suffix::register_suffixes(BEd &ctx) {
  ctx.suffixes['p' - 'a'] = Suffix{
    .desc = "Prints current line.",
    .handle = [](BEd &ctx) {
      auto &addr = ctx.current();
      ctx.buffer(addr.buffername).print(ctx, addr.number, addr.number);
    }
  };
  ctx.suffixes['n' - 'a'] = Suffix{
    .desc = "Prints current line with line number.",
    .handle = [](BEd &ctx) {
      auto &addr = ctx.current();
      ctx.buffer(addr.buffername).number_print(ctx, addr.number, addr.number);
    }
  };
}

void Function::register_posix(BEd &ctx) {
  ctx.no_op = Function{
    .address_kind = Function::AddressKind::Line,
    .argument_kind = Function::ArgumentKind::None,
    .input_mode = Function::InputMode::None,
    .desc = "Prints a line and jumps to it.",
    .default_address = ".+1",
    .accept_zero = false,
    .pre_text_mode = nullptr,
    .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
      auto addr = std::get<buffer::Line>(addr_);
      ctx.buffer(addr.buffername).print(ctx, addr.number, addr.number);
      ctx.current() = addr;
    }
  };
  ctx.eof_op = Function{
    .address_kind = Function::AddressKind::None,
    .argument_kind = Function::ArgumentKind::None,
    .input_mode = Function::InputMode::None,
    .desc = "Try quitting.",
    .default_address = "",
    .accept_zero = false,
    .pre_text_mode = nullptr,
    .handle = [](BEd &ctx, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
      for (auto &[name, buffer] : ctx.buffers) {
        if (buffer->state == buffer::Buffer::Modified) {
          buffer->state = buffer::Buffer::Warned;
          throw ed_error("Buffer " + name + " modified.");
        }
      }
      throw fatal_error("Quitting", 0);
    }
  };
  ctx.functions.insert(
    "a",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::Text,
      .desc = "Append text to a line.",
      .default_address = ".",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *text, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Line>(addr_);
        ctx.buffer(addr.buffername).append(ctx, text, addr.number);
        ctx.current() = {ctx.prev.buffername, ctx.prev.end};
        vase::Shard::release(text);
      }
    }
  );
  ctx.functions.insert(
    "j",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Join a set of lines.",
      .default_address = ".,.+1",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).join(ctx, addr.start, addr.end);
        ctx.current() = {addr.buffername, addr.start};
      }
    }
  );
  ctx.functions.insert(
    "q",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Try quitting.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        for (auto &[name, buffer] : ctx.buffers) {
          if (buffer->state == buffer::Buffer::Modified) {
            buffer->state = buffer::Buffer::Warned;
            throw ed_error("Buffer " + name + " modified.");
          }
        }
        throw fatal_error("Quitting", 0);
      }
    }
  );
  ctx.functions.insert(
    "Q",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Force quit.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        throw fatal_error("Force Quitting", 0);
      }
    }
  );
  ctx.functions.insert(
    "p",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print range",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).print(ctx, addr.start, addr.end);
        ctx.current() = {addr.buffername, addr.end};
      },
    }
  );
  ctx.functions.insert(
    "n",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print range with line numbers",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).number_print(ctx, addr.start, addr.end);
        ctx.current() = {addr.buffername, addr.end};
      },
    }
  );
  ctx.functions.insert(
    "=",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print line numbers",
      .default_address = "$",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        if (addr.start == addr.end)
          std::cout << ':' << addr.buffername << ':' << addr.start << "\n";
        else
          std::cout << ':' << addr.buffername << ':' << addr.start << "," << addr.end << "\n";
        ctx.current() = {addr.buffername, addr.end};
      },
    }
  );
  ctx.functions.insert(
    "k",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::Mark,
      .input_mode = Function::InputMode::None,
      .desc = "Mark a line.",
      .default_address = ".",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Line>(addr_);
        ctx.mark(std::get<char>(arg), addr);
      }
    }
  );
  ctx.functions.insert(
    "e",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Try load a file into the current buffer.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg, std::vector<buffer::Line> *) {
        auto &addr = std::get<std::string>(addr_);
        auto &buf = ctx.buffer(addr);
        if (buf.state == buffer::Buffer::Modified) {
          buf.state = buffer::Buffer::Warned;
          throw ed_error("Buffer modified.");
        }
        vase::Shard *s = nullptr;
        if (std::holds_alternative<std::filesystem::path>(arg)) {
          s = vase::Shard::from_file(std::get<std::filesystem::path>(arg), true);
          buf.save_path = std::get<std::filesystem::path>(arg);
        } else if (std::holds_alternative<ShellArg>(arg)) {
          s = vase::Shard::from_command(std::get<ShellArg>(arg).cmd.c_str(), true);
        } else if (std::holds_alternative<std::monostate>(arg)) {
          if (buf.save_path != "")
            s = vase::Shard::from_file(buf.save_path, true);
          else
            throw ed_error("Need filename to load.");
        }
        buf.load(ctx, s);
        std::cout << buf.bytes() << std::endl;
        ctx.current() = {addr, buf.lines()};
      }
    }
  );
  ctx.functions.insert(
    "E",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Load a file into the current buffer.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg, std::vector<buffer::Line> *) {
        auto &addr = std::get<std::string>(addr_);
        auto &buf = ctx.buffer(addr);
        vase::Shard *s = nullptr;
        if (std::holds_alternative<std::filesystem::path>(arg)) {
          s = vase::Shard::from_file(std::get<std::filesystem::path>(arg), true);
          buf.save_path = std::get<std::filesystem::path>(arg);
        } else if (std::holds_alternative<ShellArg>(arg)) {
          s = vase::Shard::from_command(std::get<ShellArg>(arg).cmd.c_str(), true);
        } else if (std::holds_alternative<std::monostate>(arg)) {
          if (buf.save_path != "")
            s = vase::Shard::from_file(buf.save_path, true);
          else
            throw ed_error("Need filename to load.");
        }
        buf.load(ctx, s);
        std::cout << buf.bytes() << std::endl;
        ctx.current() = {addr, buf.lines()};
      }
    }
  );
  ctx.functions.insert(
    "H",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Toggle help mode.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        ctx.help_mode = !ctx.help_mode;
        if (ctx.help_mode)
          std::cout << ctx.last_help << std::endl;
      }
    }
  );
  ctx.functions.insert(
    "h",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print last help message",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        std::cout << ctx.last_help << std::endl;
      }
    }
  );
  ctx.functions.insert(
    "P",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Toggle prompt.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        ctx.prompt_mode = !ctx.prompt_mode;
        if (ctx.prompt_mode && !ctx.prompt) {
          ctx.prompt = [](BEd &) { return "*"; };
        }
      }
    }
  );
}
} // namespace bed::internal::functions
