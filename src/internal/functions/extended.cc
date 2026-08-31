#include "bed.h"
#include "internal/functions/functions.h"
#include "internal/functions/suffixes.h"

namespace bed::internal::functions {
void Function::register_extented(BEd &ctx) {
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
      .handle = [](BEd &, const buffer::Address &, vase::Shard *, const Argument &arg_, std::vector<buffer::Line> *) {
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
      .handle = [](BEd &ctx, const buffer::Address &, vase::Shard *, const Argument &, std::vector<buffer::Line> *) {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd)))
          throw ed_error("Can't determine current directory.");
        ctx.io.write_line(cwd);
      },
    }
  );
}
} // namespace bed::internal::functions
