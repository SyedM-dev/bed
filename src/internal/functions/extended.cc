#include "bed.h"
#include "internal/functions/functions.h"
#include "internal/functions/suffixes.h"

namespace bed::internal::functions {
static std::string_view address_kind_str(Function::AddressKind k) {
  switch (k) {
  case Function::AddressKind::None:
    return "none";
  case Function::AddressKind::Line:
    return "1 line";
  case Function::AddressKind::Range:
    return "range";
  }
  return "";
}

static std::string_view input_mode_str(Function::InputMode m) {
  switch (m) {
  case Function::InputMode::None:
    return "";
  case Function::InputMode::Text:
    return "text";
  case Function::InputMode::Interactive:
    return "interactive";
  case Function::InputMode::CommandList:
    return "command list";
  }
  return "";
}

static std::string_view argument_kind_str(Function::ArgumentKind a) {
  switch (a) {
  case Function::ArgumentKind::None:
    return "";
  case Function::ArgumentKind::Regex:
    return "regex";
  case Function::ArgumentKind::Shell:
    return "shell command";
  case Function::ArgumentKind::Any:
    return "any";
  case Function::ArgumentKind::File:
    return "file";
  case Function::ArgumentKind::Global:
    return "ed global-style";
  case Function::ArgumentKind::Mark:
    return "mark name";
  case Function::ArgumentKind::Number:
    return "number";
  case Function::ArgumentKind::Line:
    return "address";
  case Function::ArgumentKind::Range:
    return "range";
  case Function::ArgumentKind::Ruby:
    return "ruby code";
  }
  return "";
}

static void append_field(BEd &ctx, std::string_view label, io::Token::Kind color, std::string_view value) {
  if (value.empty())
    return;
  ctx.io.write("\n");
  ctx.io.apply(color);
  ctx.io.write(label);
  ctx.io.reset();
  ctx.io.write(": ");
  ctx.io.write(value);
}

static void describe_function(BEd &ctx, const Function &func) {
  ctx.io.write(func.desc);
  ctx.io.reset();
  append_field(ctx, "Default address", io::Token::Color1, func.default_address);
  append_field(ctx, "Address", io::Token::Color2, address_kind_str(func.address_kind));
  if (func.accept_zero)
    append_field(ctx, "Zero address", io::Token::Color3, "allowed");
  append_field(ctx, "Input", io::Token::Color4, input_mode_str(func.input_mode));
  append_field(ctx, "Argument", io::Token::Color5, argument_kind_str(func.argument_kind));
  ctx.io.write("\n");
}

void Function::register_extented(BEd &ctx) {
  ctx.functions.insert(
    "*",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::Any,
      .input_mode = Function::InputMode::None,
      .desc = "Explain a command",
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
        auto str = std::get<std::string>(arg_);
        const auto first = str.find_first_not_of(" \t");
        const auto last = str.find_last_not_of(" \t");
        if (first == std::string::npos)
          str.clear();
        else
          str = str.substr(first, last - first + 1);
        if (str.empty()) {
          describe_function(ctx, ctx.no_op);
          return;
        }
        auto len = ctx.functions.longest_match(str);
        if (len == str.size()) {
          describe_function(ctx, *ctx.functions.get_ptr(str));
          return;
        }
        throw ed_error("Not a valid function name.");
      },
    }
  );
  ctx.functions.insert(
    "b",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "List active buffers",
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
        auto &current = std::get<std::string>(addr_);
        bool current_real = false;
        for (auto &[name, _] : ctx.buffers) {
          if (current == name) {
            ctx.io.write("* ");
            current_real = true;
          } else {
            ctx.io.write("  ");
          }
          ctx.io.write_line(name);
        }
        if (!current_real)
          ctx.io.write_line("* " + current);
      },
    }
  );
  ctx.functions.insert(
    "#",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::Any,
      .input_mode = Function::InputMode::None,
      .desc = "Write a comment",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &,
                  std::vector<buffer::Line> *
                ) {},
    }
  );
  ctx.functions.insert(
    "echo",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::Any,
      .input_mode = Function::InputMode::None,
      .desc = "Echo the given message (replacing $1-$4 with address information)",
      .default_address = "",
      .accept_zero = true,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &ctx,
                  const buffer::Address &addr_,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
        auto &addr = std::get<buffer::Range>(addr_);
        auto str = std::get<std::string>(arg_);
        const auto first = str.find_first_not_of(" \t");
        const auto last = str.find_last_not_of(" \t");
        if (first == std::string::npos)
          str.clear();
        else
          str = str.substr(first, last - first + 1);
        for (size_t i = 0; i < str.size();) {
          if (str[i] == '\\') {
            if (i + 1 >= str.size())
              break;
            str.erase(i++, 1);
            if (str[i - 1] == 'n')
              str[i - 1] = '\n';
            continue;
          }
          if (str[i] == '$' && i < str.size() && '1' <= str[i + 1] && str[i + 1] <= '4') {
            char c = str[i + 1];
            str.erase(i, 2);
            switch (c) {
            case '1': {
              auto start = std::to_string(addr.start);
              str.insert(i, start);
              i += start.size();
            } break;
            case '2': {
              auto end = std::to_string(addr.end);
              str.insert(i, end);
              i += end.size();
            } break;
            case '3': {
              str.insert(i, addr.buffername);
              i += addr.buffername.size();
            } break;
            case '4': {
              auto &buf = ctx.buffer(addr.buffername);
              auto filename = buf.filename().string();
              str.insert(i, filename);
              i += filename.size();
            } break;
            }
            continue;
          }
          i++;
        }
        ctx.io.write_line(str);
      },
    }
  );
  ctx.functions.insert(
    "cd",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::Any,
      .input_mode = Function::InputMode::None,
      .desc = "Change directory",
      .default_address = "",
      .accept_zero = false,
      .pre_text_mode = nullptr,
      .handle = [](
                  BEd &,
                  const buffer::Address &,
                  vase::Shard *,
                  const Argument &arg_,
                  std::vector<buffer::Line> *
                ) {
        auto path = std::get<std::string>(arg_);
        const auto first = path.find_first_not_of(" \t");
        const auto last = path.find_last_not_of(" \t");
        if (first == std::string::npos)
          path.clear();
        else
          path = path.substr(first, last - first + 1);
        if (path.empty())
          path = getenv("HOME");
        if (path == "~")
          path = getenv("HOME");
        if (path.starts_with("~/")) {
          const char *home = getenv("HOME");
          if (home)
            path = std::string(home) + path.substr(1);
        }
        if (chdir(path.c_str()) == -1)
          throw ed_error("Can't change directory.");
      },
    }
  );
  ctx.functions.insert(
    "pwd",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Print the current working directory",
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
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd)))
          throw ed_error("Can't determine current directory.");
        ctx.io.write_line(cwd);
      },
    }
  );
  ctx.functions.insert(
    "x",
    Function{
      .address_kind = Function::AddressKind::Range,
      .argument_kind = Function::ArgumentKind::Range,
      .input_mode = Function::InputMode::None,
      .desc = "Exchange a range of lines for another",
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
        auto arg = std::get<buffer::Range>(arg_);
        auto text = ctx.buffer(addr.buffername).copy(addr.start, addr.end);
        try {
          ctx.buffer(arg.buffername).replace(ctx, text, arg.start, arg.end);
          vase::Shard::release(text);
        } catch (...) {
          vase::Shard::release(text);
          throw;
        }
      },
    }
  );
  ctx.functions.insert(
    "U",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "Redo the last undo modification",
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
          throw ed_error("Can't redo buffer");
        auto &buf = *(buffer::GenericBuffer *)&buf_;
        if (!buf.redo(ctx))
          throw ed_error("Can't redo buffer.");
      }
    }
  );
  ctx.functions.insert(
    "hl",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::None,
      .input_mode = Function::InputMode::None,
      .desc = "List available history versions",
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
          throw ed_error("Can't redo buffer");
        auto &buf = *(buffer::GenericBuffer *)&buf_;
        buf.list_history(ctx);
      }
    }
  );
  ctx.functions.insert(
    "hp",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::Number,
      .input_mode = Function::InputMode::None,
      .desc = "Prune old history versions",
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
        auto &arg = std::get<int64_t>(arg_);
        auto &buf_ = ctx.buffer(addr);
        if (buf_.kind != buffer::Buffer::Kind::Generic)
          throw ed_error("Can't prune buffer.");
        auto &buf = *(buffer::GenericBuffer *)&buf_;
        auto versions = buf.prune(arg);
        if (!ctx.suppress_mode)
          ctx.io.write_line(std::format("{} undo versions left.", versions));
      }
    }
  );
}
} // namespace bed::internal::functions
