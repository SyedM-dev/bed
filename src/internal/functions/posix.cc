#include "bed.h"
#include "internal/functions/functions.h"
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
  ctx.suffixes['l' - 'a'] = Suffix{
    .desc = "Prints current line unambiguously.",
    .handle = [](BEd &ctx) {
      auto &addr = ctx.current();
      ctx.buffer(addr.buffername).list_print(ctx, addr.number, addr.number);
    }
  };
}

void Function::register_posix(BEd &ctx) {
  ctx.functions.insert(
    "a",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::Text,
      .desc = "Append text to a line",
      .default_address = ".",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *text,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto addr = std::get<buffer::Line>(addr_);
        ctx.buffer(addr.buffername).append(ctx, text, addr.number);
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
      .desc = "Change a range of lines",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *text,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).replace(ctx, text, addr.start, addr.end);
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
      .desc = "Delete a range of lines",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).remove(ctx, addr.start, addr.end);
      }
    }
  );
  ctx.functions.insert(
    "e",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Load a file into the current buffer, warn once if unsaved changes exist",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg,
                  std::vector<buffer::Line> *
                ) {
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
          ctx.escape_command(cmd, buf.filename().string());
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
        if (!ctx.suppress_mode)
          ctx.io.write_line(std::format("{}", buf.bytes()));
      }
    }
  );
  ctx.functions.insert(
    "E",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Load a file into the current buffer, discarding unsaved changes",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg,
                  std::vector<buffer::Line> *
                ) {
        auto &addr = std::get<std::string>(addr_);
        auto &buf = ctx.buffer(addr);
        vase::Shard *s = nullptr;
        if (std::holds_alternative<std::filesystem::path>(arg)) {
          auto path = std::get<std::filesystem::path>(arg);
          s = vase::Shard::from_file(path, true);
          buf.set_filename(path);
        } else if (std::holds_alternative<ShellArg>(arg)) {
          auto cmd = std::get<ShellArg>(arg).cmd;
          ctx.escape_command(cmd, buf.filename().string());
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
        if (!ctx.suppress_mode)
          ctx.io.write_line(std::format("{}", buf.bytes()));
      }
    }
  );
  ctx.functions.insert(
    "f",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Set/print a save path",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg,
                  std::vector<buffer::Line> *
                ) {
        auto &addr = std::get<std::string>(addr_);
        auto &buf = ctx.buffer(addr);
        if (std::holds_alternative<std::filesystem::path>(arg))
          buf.set_filename(std::get<std::filesystem::path>(arg));
        else if (std::holds_alternative<ShellArg>(arg))
          throw ed_error("Can't save shell command as save path.");
        if (buf.filename().empty())
          throw ed_error("Filename needed.");
        ctx.io.write_line(buf.filename().string());
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
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        ctx.io.apply(internal::io::Token::Warning);
        ctx.io.write_line(ctx.last_help);
        ctx.io.reset();
      }
    }
  );
  ctx.functions.insert(
    "H",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Toggle help mode",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        ctx.help_mode = !ctx.help_mode;
        ctx.io.apply(internal::io::Token::Warning);
        if (ctx.help_mode)
          ctx.io.write_line(ctx.last_help);
        ctx.io.reset();
      }
    }
  );
  ctx.functions.insert(
    "i",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::Text,
      .desc = "Insert text before a line",
      .default_address = ".",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *text,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto addr = std::get<buffer::Line>(addr_);
        if (addr.number)
          addr.number--;
        ctx.buffer(addr.buffername).append(ctx, text, addr.number);
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
      .desc = "Join a range of lines",
      .default_address = ".,.+1",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto addr = std::get<buffer::Range>(addr_);
        ctx.buffer(addr.buffername).join(ctx, addr.start, addr.end);
      }
    }
  );
  ctx.functions.insert(
    "k",
    Function{
      .address_kind = Function::AddressKind::Line,
      .argument_kind = Function::ArgumentKind::Mark,
      .input_mode = Function::InputMode::None,
      .desc = "Mark a line",
      .default_address = ".",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg,
                  std::vector<buffer::Line> *
                ) {
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
      .desc = "Print a range, showing nonprinting characters unambiguously",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
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
      .desc = "Move a range of lines",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
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
      },
    }
  );
  ctx.functions.insert(
    "n",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print a range with line numbers",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
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
      .desc = "Print a range",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
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
      .argument_kind = Function::ArgumentKind::Any,
      .input_mode = Function::InputMode::None,
      .desc = "Toggle/set prompt string",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
        auto &arg = std::get<std::string>(arg_);
        if (arg.size()) {
          ctx.prompt_mode = true;
          ctx.prompt = [arg](BEd &) { return arg; };
          return;
        }
        ctx.prompt_mode = !ctx.prompt_mode;
        if (ctx.prompt_mode && !ctx.prompt)
          ctx.prompt = [](BEd &) { return "*"; };
      }
    }
  );
  ctx.functions.insert(
    "q",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Quit, warn once if unsaved changes exist",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
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
      .desc = "Quit without saving, discarding unsaved changes",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
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
      .desc = "Read a file's contents into the buffer after the given address",
      .default_address = "$",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg,
                  std::vector<buffer::Line> *
                ) {
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
          ctx.escape_command(cmd, buf.filename().string());
          s = vase::Shard::from_command(cmd.c_str(), true);
        } else {
          auto path = buf.filename();
          if (path.empty())
            throw ed_error("Need filename.");
          s = vase::Shard::from_file(path, true);
        };
        try {
          buf.append(ctx, s, addr.number);
          vase::Shard::release(s);
        } catch (...) {
          vase::Shard::release(s);
          throw;
        }
        if (!ctx.suppress_mode)
          ctx.io.write_line(std::format("{}", s ? s->length + 1 : 0));
      }
    }
  );
  ctx.functions.insert(
    "s",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::Regex,
      .input_mode = Function::InputMode::None,
      .desc = "Substitute pattern in range for replacement",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
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
      .desc = "Copy a range of lines",
      .default_address = ".,.",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
        auto addr = std::get<buffer::Range>(addr_);
        auto arg = std::get<buffer::Line>(arg_);
        auto text = ctx.buffer(addr.buffername).copy(addr.start, addr.end);
        try {
          ctx.buffer(arg.buffername).append(ctx, text, arg.number);
          vase::Shard::release(text);
        } catch (...) {
          vase::Shard::release(text);
          throw;
        }
      },
    }
  );
  ctx.functions.insert(
    "u",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Undo the last modification to the buffer",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto &addr = std::get<std::string>(addr_);
        auto &buf_ = ctx.buffer(addr);
        if (buf_.kind != buffer::Buffer::Kind::Generic)
          throw ed_error("Can't undo buffer");
        auto &buf = *(buffer::GenericBuffer *)&buf_;
        if (!buf.undo(ctx))
          throw ed_error("Can't undo buffer.");
      }
    }
  );
  ctx.functions.insert(
    "w",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::File,
      .input_mode = Function::InputMode::None,
      .desc = "Write a range of lines to disk",
      .default_address = "0,$",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg,
                  std::vector<buffer::Line> *
                ) {
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
            ctx.escape_command(cmd, buf.filename().string());
            vase::write_command(cmd.c_str(), text);
          } else {
            auto path = buf.filename();
            if (path.empty())
              throw ed_error("Need filename.");
            vase::write_file(path, text);
          };
          vase::Shard::release(text);
        } catch (...) {
          vase::Shard::release(text);
          throw;
        }
        if (!ctx.suppress_mode)
          ctx.io.write(std::format("{}\n", text ? text->length + 1 : 0));
      }
    }
  );
  ctx.functions.insert(
    "=",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print the line number of the given address",
      .default_address = "$",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {
        auto &addr = std::get<buffer::Range>(addr_);
        ctx.io.apply(io::Token::BufferName);
        ctx.io.write(":" + addr.buffername + ":");
        ctx.io.apply(io::Token::Number);
        if (addr.start == addr.end) {
          ctx.io.write_line(std::format(" {}", addr.start));
        } else {
          ctx.io.write(std::format(" {}", addr.start));
          ctx.io.apply(io::Token::AddressSeperator);
          ctx.io.write(",");
          ctx.io.apply(io::Token::Number);
          ctx.io.write_line(std::format("{}", addr.end));
        }
        ctx.io.reset();
        ctx.prev().buffername = addr.buffername;
        ctx.prev().start = addr.start;
        ctx.prev().end = addr.end;
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
      .desc = "Run a shell command",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
        auto &addr = std::get<std::string>(addr_);
        auto filename = ctx.buffer(addr).filename();
        auto &arg = std::get<ShellArg>(arg_);
        auto cmd = arg.cmd;
        if (ctx.escape_command(cmd, filename.string()))
          ctx.io.write(cmd + "\n");
        ctx.io.run_pty(cmd);
        if (!ctx.suppress_mode)
          ctx.io.write("!\n");
      },
    }
  );
  ctx.no_op = Function{
    .address_kind = Function::AddressKind::Line,
    .argument_kind = Function::ArgumentKind::None,
    .input_mode = Function::InputMode::None,
    .desc = "Print given line and move the cursor to it",
    .default_address = ".+1",
    .accept_zero = true,
    .pre_text_mode = nullptr,
    .handle = [](
                BEd &ctx,
                const buffer::Address &addr_,
                vase::Shard *,
                const Argument &,
                std::vector<buffer::Line> *
              ) {
      auto addr = std::get<buffer::Line>(addr_);
      if (addr.number != 0)
        ctx.buffer(addr.buffername).print(ctx, addr.number, addr.number);
      ctx.current() = addr;
      ctx.prev().buffername = addr.buffername;
      ctx.prev().start = addr.number;
      ctx.prev().end = addr.number;
    }
  };
  ctx.eof_op = Function{
    .address_kind = Function::AddressKind::None,
    .argument_kind = Function::ArgumentKind::None,
    .input_mode = Function::InputMode::None,
    .desc = "Quit, warn once if unsaved changes exist",
    .default_address = "",
    .accept_zero = false,
    .pre_text_mode = nullptr,
    .handle = [](
                BEd &ctx,
                const buffer::Address &,
                vase::Shard *,
                const Argument &,
                std::vector<buffer::Line> *
              ) {
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
  };
}
} // namespace bed::internal::functions
