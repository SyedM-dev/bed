#include "bed.h"
#include "internal/functions/functions.h"
#include "internal/functions/suffixes.h"

namespace bed::internal::functions {
void Function::register_extented(BEd &ctx) {
  ctx.functions.insert(
    "#",
    Function{
      .address_kind = Function::AddressKind::None,
      .argument_kind = Function::ArgumentKind::Any,
      .input_mode = Function::InputMode::None,
      .desc = "Comment.",
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
      .desc = "Echo given message.",
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
          if (str[i] == '$' && i < str.size() && '1' <= str[i + 1] && str[i + 1] <= '2') {
            bool one = str[i + 1] == '1';
            str.erase(i, 2);
            if (one) {
              auto start = std::to_string(addr.start);
              str.insert(i, start);
              i += start.size();
            } else {
              auto end = std::to_string(addr.end);
              str.insert(i, end);
              i += end.size();
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
      .desc = "Change directory.",
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
      .desc = "Print directory.",
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
      .desc = "Exchange a range of lines for another.",
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
      .desc = "Redo last undo.",
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
      .desc = "List history versions.",
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
}
} // namespace bed::internal::functions
