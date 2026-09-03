#include "bed.h"
#include "internal/parser/parser.h"

namespace bed {
BEd::BEd(std::vector<std::string> args, internal::io::IO &io)
    : theme(internal::theme::Theme::default_theme()), io(io) {
  internal::functions::Function::register_posix(*this);
  internal::functions::Function::register_extented(*this);
  internal::functions::Suffix::register_suffixes(*this);
  languages["ruby"] = new internal::syntax::Language(internal::syntax::ruby::lang_ruby());
  std::string prompt_ = "";
  std::string file = "";
  bool suppress = false;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p") {
      i++;
      if (i >= args.size())
        throw fatal_error("Prompt not specified!", 1);
      prompt_ = args[i];
    } else if (args[i] == "-s") {
      suppress = true;
    } else if (args[i] == "-v" || args[i] == "--verbose") {
      help_mode = true;
    } else {
      if (file.size())
        throw fatal_error("Invalid arguments given.", 1);
      file = args[i];
    }
  }
  if (prompt_ != "")
    prompt = [p = std::move(prompt_)](BEd &) { return p; };
  else
    prompt_mode = false;
  suppress_mode = suppress;
  buffers["clip"] = new internal::buffer::ClipBuffer("clip");
  current() = {"default", 0};
  try {
    if (file != "")
      handle(":default:E " + file, false);
  } catch (ed_error &e) {
    io.write_line("?");
    if (help_mode)
      io.write_line(e.what());
    last_help = e.what();
  }
}

BEd::~BEd() {
  for (auto &[_, buffer] : buffers)
    delete buffer;
  for (auto &[_, lang] : languages)
    delete lang;
}

void BEd::run() {
  while (true) {
    internal::ui::CommandIO command(*this);
    auto [cmd, eof] = command.run();
    try {
      handle(cmd, eof);
    } catch (ed_error &e) {
      io.write_line("?");
      if (help_mode)
        io.write_line(e.what());
      last_help = e.what();
    }
  }
}

void BEd::handle(std::string_view cmd, bool eof) {
  if (eof) {
    eof_op.handle(*this, "", nullptr, std::monostate(), nullptr);
    return;
  }
  internal::parser::Command c = internal::parser::Parser::get_command(cmd, *this);
  if (c.temp_address) {
    marks.get(251) = marks.get(250);
    prev_2 = prev_1;
    temporary_current = true;
  }
  internal::buffer::Address address;
  switch (c.function->address_kind) {
  case internal::functions::Function::AddressKind::None: {
    auto a = internal::parser::AddressPromise::get_line(*this, c.addresses);
    if (!a.has_value()) {
      auto vec = internal::parser::Parser::get_addresses(c.function->default_address, *this);
      a = internal::parser::AddressPromise::get_line(*this, vec);
      if (!a.has_value())
        a = current();
    }
    address = a->buffername;
  } break;
  case internal::functions::Function::AddressKind::Line: {
    auto a = internal::parser::AddressPromise::get_line(*this, c.addresses);
    if (!a.has_value()) {
      auto vec = internal::parser::Parser::get_addresses(c.function->default_address, *this);
      a = internal::parser::AddressPromise::get_line(*this, vec);
      if (!a.has_value())
        a = current();
    }
    address = *a;
  } break;
  case internal::functions::Function::AddressKind::Range: {
    auto a = internal::parser::AddressPromise::get_range(*this, c.addresses);
    if (!a.has_value()) {
      auto vec = internal::parser::Parser::get_addresses(c.function->default_address, *this);
      a = internal::parser::AddressPromise::get_range(*this, vec);
      if (!a.has_value())
        a = internal::buffer::Range(current(), current());
    }
    address = *a;
  } break;
  }
  if (std::holds_alternative<internal::buffer::Line>(c.argument)) {
    if (c.argument_addresses.empty())
      throw ed_error("Function needs address argument.");
    auto a = internal::parser::AddressPromise::get_line(*this, c.argument_addresses);
    if (a.has_value())
      c.argument = *a;
    else
      c.argument = current();
  } else if (std::holds_alternative<internal::buffer::Range>(c.argument)) {
    if (c.argument_addresses.empty())
      throw ed_error("Function needs address argument.");
    auto a = internal::parser::AddressPromise::get_range(*this, c.argument_addresses);
    if (a.has_value())
      c.argument = *a;
    else
      c.argument = internal::buffer::Range(current(), current());
  }
  if (!c.function->accept_zero) {
    if (std::holds_alternative<internal::buffer::Line>(address)) {
      if (std::get<internal::buffer::Line>(address).number == 0)
        throw ed_error("Line number can't be zero.");
    } else if (std::holds_alternative<internal::buffer::Range>(address)) {
      auto r = std::get<internal::buffer::Range>(address);
      if (r.start == 0 || r.end == 0)
        throw ed_error("Line number can't be zero.");
    }
  }
  internal::vase::Shard *text = nullptr;
  if (c.function->input_mode == internal::functions::Function::InputMode::Text) {
    internal::ui::TextMode tm(*this);
    auto [a, b] = tm.run();
    if (!b)
      text = a;
  }
  if (c.function->handle)
    c.function->handle(*this, address, text, c.argument, nullptr);
  if (c.suffix)
    c.suffix->handle(*this);
  if (c.temp_address)
    temporary_current = false;
  for (auto it = buffers.begin(); it != buffers.end();) {
    internal::buffer::Buffer *buf = it->second;
    if (buf->waste()) {
      delete buf;
      it = buffers.erase(it);
    } else {
      ++it;
    }
  }
}

internal::buffer::Buffer &BEd::buffer(const std::string &name) {
  if (name.empty())
    throw ed_error("can't have empty buffer name");
  {
    auto it = buffers.find(name);
    if (it != buffers.end())
      return *it->second;
  }
  constexpr std::string_view prefix = "history/";
  if (name.starts_with(prefix)) {
    std::string_view path{name};
    path.remove_prefix(prefix.size());
    auto slash = path.rfind('_');
    if (slash == std::string_view::npos || slash == 0 || slash == path.size() - 1)
      throw ed_error("invalid history");
    std::string bufname(path.substr(0, slash));
    auto version_str = path.substr(slash + 1);
    std::size_t version;
    try {
      version = std::stoull(std::string(version_str));
    } catch (...) {
      throw ed_error("invalid history version");
    }
    auto it = buffers.find(bufname);
    if (it != buffers.end()) {
      auto &buf_ = *it->second;
      if (buf_.kind != internal::buffer::Buffer::Kind::Generic)
        throw ed_error("Only normal buffers can have history.");
      auto &buf = *(internal::buffer::GenericBuffer *)&buf_;
      auto *history_buf = buf.get_history(version);
      buffers.emplace(name, history_buf);
      return *history_buf;
    }
    throw ed_error("Buffer has no history.");
  }
  for (auto &c : name)
    if (!(('0' <= c && c <= '9')
          || ('a' <= c && c <= 'z')
          || ('A' <= c && c <= 'Z')
          || c == '-' || c == '_'
          || c == '+' || c == '.'
          || c == ',' || c == '$'
          || c == '/' || c == '~'))
      throw ed_error("Invalid buffer name.");
  auto *buf = new internal::buffer::GenericBuffer(name);
  buffers.emplace(name, buf);
  return *buf;
}

internal::buffer::Line &BEd::current() {
  return marks.get(250 + temporary_current);
}

internal::buffer::Range &BEd::prev() {
  if (temporary_current)
    return prev_2;
  else
    return prev_1;
}

void BEd::mark(uint8_t m, internal::buffer::Line line) {
  marks.get(m) = line;
}

bool BEd::escape_command(std::string &cmd, std::string_view filename) {
  bool modified = false;
  if (cmd == "!") {
    cmd = last_shell;
    modified = true;
  }
  last_shell = cmd;
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
} // namespace bed
