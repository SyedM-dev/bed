#include "internal/functions/functions.h"
#include "bed.h"
#include "internal/functions/suffixes.h"

namespace bed::internal::functions {
static bool escape_command(BEd &ctx, std::string &cmd, std::string_view filename) {
  bool modified = false;
  if (cmd == "!") {
    cmd = ctx.last_shell;
    modified = true;
  }
  ctx.last_shell = cmd;
  for (size_t i = 0; i < cmd.size();) {
    if (cmd[i] == '\\') {
      if (i + 1 >= cmd.size())
        break;
      cmd.erase(i++, 1);
      continue;
    }
    if (cmd[i] == '%') {
      cmd.erase(i, 1);
      cmd.insert(i, filename);
      i += filename.size();
      modified = true;
      continue;
    }
    i++;
  }
  return modified;
}

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
  ctx.suffixes['l' - 'a'] = Suffix{
    .desc = "Prints current line unambiguously.",
    .handle = [](BEd &ctx) {
      auto &addr = ctx.current();
      ctx.buffer(addr.buffername).list_print(ctx, addr.number, addr.number);
    }
  };
}

void Function::register_posix(BEd &ctx) {
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
        ctx.current() = {ctx.prev().buffername, ctx.prev().end};
        vase::Shard::release(text);
      }
    }
  );
  ctx.functions.insert(
    "c",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::Text,
      .desc = "Change set of lines.",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *text, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).remove(ctx, addr.start, addr.end);
        ctx.buffer(addr.buffername).append(ctx, text, addr.start - 1);
        ctx.current() = {ctx.prev().buffername, ctx.prev().end};
        vase::Shard::release(text);
      }
    }
  );
  ctx.functions.insert(
    "d",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Delete set of lines.",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).remove(ctx, addr.start, addr.end);
        ctx.current() = {addr.buffername, addr.start};
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
          auto path = std::get<std::filesystem::path>(arg);
          s = vase::Shard::from_file(path, true);
          buf.set_filename(path);
        } else if (std::holds_alternative<ShellArg>(arg)) {
          auto cmd = std::get<ShellArg>(arg).cmd;
          escape_command(ctx, cmd, buf.filename().string());
          s = vase::Shard::from_command(cmd.c_str(), true);
        } else {
          auto path = buf.filename();
          if (path.empty())
            throw ed_error("Need filename.");
          s = vase::Shard::from_file(path, true);
        };
        try {
          buf.load(ctx, s);
          vase::Shard::release(s);
        } catch (...) {
          vase::Shard::release(s);
          throw;
        }
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
          auto path = std::get<std::filesystem::path>(arg);
          s = vase::Shard::from_file(path, true);
          buf.set_filename(path);
        } else if (std::holds_alternative<ShellArg>(arg)) {
          auto cmd = std::get<ShellArg>(arg).cmd;
          escape_command(ctx, cmd, buf.filename().string());
          s = vase::Shard::from_command(cmd.c_str(), true);
        } else {
          auto path = buf.filename();
          if (path.empty())
            throw ed_error("Need filename.");
          s = vase::Shard::from_file(path, true);
        };
        try {
          buf.load(ctx, s);
          vase::Shard::release(s);
        } catch (...) {
          vase::Shard::release(s);
          throw;
        }
        std::cout << buf.bytes() << std::endl;
        ctx.current() = {addr, buf.lines()};
      }
    }
  );
  ctx.functions.insert(
    "f",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Set a save path.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg, std::vector<buffer::Line> *) {
        auto &addr = std::get<std::string>(addr_);
        auto &buf_ = ctx.buffer(addr);
        if (buf_.kind != buffer::Buffer::Kind::Generic)
          return;
        auto &buf = *(buffer::GenericBuffer *)&buf_;
        if (std::holds_alternative<std::filesystem::path>(arg))
          buf.set_filename(std::get<std::filesystem::path>(arg));
        else if (std::holds_alternative<ShellArg>(arg))
          throw ed_error("Can't save shell command as save path.");
        if (buf.filename().empty())
          throw ed_error("Filename needed.");
        std::cout << buf.filename() << std::endl;
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
    "i",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::Text,
      .desc = "Insert text before a line.",
      .default_address = ".",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *text, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Line>(addr_);
        if (addr.number)
          addr.number--;
        ctx.buffer(addr.buffername).append(ctx, text, addr.number);
        ctx.current() = {ctx.prev().buffername, ctx.prev().end};
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
        auto &addr = std::get<buffer::Line>(addr_);
        ctx.mark(std::get<char>(arg), addr);
      }
    }
  );
  ctx.functions.insert(
    "l",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "List (print unambiguous) range",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).list_print(ctx, addr.start, addr.end);
        ctx.current() = {addr.buffername, addr.end};
      },
    }
  );
  ctx.functions.insert(
    "m",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::Line,
      .input_mode = Function::InputMode::None,
      .desc = "Move a range of lines.",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg_, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        auto arg = std::get<buffer::Line>(arg_);
        if (arg.buffername == addr.buffername
            && addr.start <= arg.number
            && addr.end < arg.number)
          throw ed_error("Can't move lines within themselves.");
        auto text = ctx.buffer(addr.buffername).copy(addr.start, addr.end);
        ctx.mark(252, arg);
        ctx.buffer(addr.buffername).remove(ctx, addr.start, addr.end);
        arg = ctx.marks.get(252);
        try {
          if (arg.number == UINT64_MAX)
            throw ed_error("Unexpected error when moving lines.");
          ctx.buffer(arg.buffername).append(ctx, text, arg.number);
          vase::Shard::release(text);
        } catch (...) {
          if (!addr.start) {
            vase::Shard::release(text);
            throw;
          }
          ctx.buffer(addr.buffername).append(ctx, text, --addr.start);
          vase::Shard::release(text);
          throw;
        }
        ctx.current() = {arg.buffername, arg.number + addr.end - addr.start + 1};
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
        std::string modified_buffers;
        for (auto &[name, buffer] : ctx.buffers) {
          if (buffer->state == buffer::Buffer::Modified) {
            buffer->state = buffer::Buffer::Warned;
            modified_buffers.append(name + ", ");
          }
        }
        if (!modified_buffers.size())
          throw fatal_error("Quitting", 0);
        modified_buffers.erase(modified_buffers.size() - 2);
        throw ed_error("Buffer(s) " + modified_buffers + " modified.");
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
    "r",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Read from file into buffer.",
      .default_address = "$",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg, std::vector<buffer::Line> *) {
        auto &addr = std::get<buffer::Line>(addr_);
        auto &buf = ctx.buffer(addr.buffername);
        vase::Shard *s = nullptr;
        if (std::holds_alternative<std::filesystem::path>(arg)) {
          auto path = std::get<std::filesystem::path>(arg);
          s = vase::Shard::from_file(path, true);
          if (buf.filename().empty())
            buf.set_filename(path);
        } else if (std::holds_alternative<ShellArg>(arg)) {
          auto cmd = std::get<ShellArg>(arg).cmd;
          escape_command(ctx, cmd, buf.filename().string());
          s = vase::Shard::from_command(cmd.c_str(), true);
        } else {
          auto path = buf.filename();
          if (path.empty())
            throw ed_error("Need filename.");
          s = vase::Shard::from_file(path, true);
        };
        try {
          buf.append(ctx, s, addr.number);
          std::cout << (s ? s->length + 1 : 0) << std::endl;
          ctx.current() = {addr.buffername, addr.number + (s ? s->lines + 1 : 0)};
          vase::Shard::release(s);
        } catch (...) {
          vase::Shard::release(s);
          throw;
        }
      }
    }
  );
  ctx.functions.insert(
    "s",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::Regex,
      .input_mode = Function::InputMode::None,
      .desc = "Substitute regex.",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg_, std::vector<buffer::Line> *) {
        auto &addr = std::get<buffer::Range>(addr_);
        auto arg = std::get<RegexArg>(arg_);
        if (arg.expression == "")
          arg.expression = ctx.last_regex;
        else
          ctx.last_regex = arg.expression;
        if (arg.replacement == "%")
          arg.replacement = ctx.last_replacement;
        else
          ctx.last_replacement = arg.replacement;
        ctx.buffer(addr.buffername)
          .substitute(
            ctx,
            addr.start,
            addr.end,
            arg.expression,
            arg.replacement,
            arg.options
          );
      }
    }
  );
  ctx.functions.insert(
    "t",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::Line,
      .input_mode = Function::InputMode::None,
      .desc = "Copy a range of lines.",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg_, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        auto arg = std::get<buffer::Line>(arg_);
        auto text = ctx.buffer(addr.buffername).copy(addr.start, addr.end);
        try {
          ctx.buffer(arg.buffername).append(ctx, text, arg.number);
          vase::Shard::release(text);
        } catch (...) {
          if (!addr.start) {
            vase::Shard::release(text);
            throw;
          }
          ctx.buffer(addr.buffername).append(ctx, text, --addr.start);
          vase::Shard::release(text);
          throw;
        }
        ctx.current() = {arg.buffername, arg.number + addr.end - addr.start + 1};
      },
    }
  );
  ctx.functions.insert(
    "w",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Write buffer to disk.",
      .default_address = "0,$",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg, std::vector<buffer::Line> *) {
        auto addr = std::get<buffer::Range>(addr_);
        auto &buf = ctx.buffer(addr.buffername);
        vase::Shard *text;
        if (!addr.start && !addr.end) {
          text = nullptr;
        } else {
          if (!addr.start && addr.end)
            addr.start = 1;
          text = buf.copy(addr.start, addr.end);
        }
        try {
          if (std::holds_alternative<std::filesystem::path>(arg)) {
            auto path = std::get<std::filesystem::path>(arg);
            vase::write_file(path, text);
            buf.set_filename(path);
          } else if (std::holds_alternative<ShellArg>(arg)) {
            auto cmd = std::get<ShellArg>(arg).cmd;
            escape_command(ctx, cmd, buf.filename().string());
            vase::write_command(cmd.c_str(), text);
          } else {
            auto path = buf.filename();
            if (path.empty())
              throw ed_error("Need filename.");
            vase::write_file(path, text);
          };
          ctx.io.write(std::format("{}\n", text ? text->length + 1 : 0));
          vase::Shard::release(text);
        } catch (...) {
          vase::Shard::release(text);
          throw;
        }
      }
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
        auto &addr = std::get<buffer::Range>(addr_);
        if (addr.start == addr.end)
          std::cout << ':' << addr.buffername << ':' << addr.start << "\n";
        else
          std::cout << ':' << addr.buffername << ':' << addr.start << "," << addr.end << "\n";
        ctx.current() = {addr.buffername, addr.end};
      },
    }
  );
  ctx.functions.insert(
    "!",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::Shell,
      .input_mode = Function::InputMode::None,
      .desc = "Run a shell command.",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &arg_, std::vector<buffer::Line> *) {
        auto &addr = std::get<std::string>(addr_);
        auto filename = ctx.buffer(addr).filename();
        auto &arg = std::get<ShellArg>(arg_);
        auto cmd = arg.cmd;
        if (escape_command(ctx, cmd, filename.string()))
          ctx.io.write(cmd + "\n");
        ctx.io.run_pty(cmd);
        ctx.io.write("!\n");
      },
    }
  );
  ctx.no_op = Function{
    .address_kind = Function::AddressKind::Line,
    .argument_kind = Function::ArgumentKind::None,
    .input_mode = Function::InputMode::None,
    .desc = "Prints a line and jumps to it.",
    .default_address = ".+1",
    .accept_zero = true,
    .pre_text_mode = nullptr,
    .handle = [](BEd &ctx, const buffer::Address &addr_, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
      auto addr = std::get<buffer::Line>(addr_);
      if (addr.number != 0)
        ctx.buffer(addr.buffername).print(ctx, addr.number, addr.number);
      ctx.current() = addr;
    }
  };
}
} // namespace bed::internal::functions
