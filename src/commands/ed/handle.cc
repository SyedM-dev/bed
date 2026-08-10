#include "commands/ed/ed.h"

namespace crib::commands::ed {
bool Ed::handle(std::string cmd, bool eof) {
  using namespace crib::internal::vase;
  try {
    if (line > vase.lines())
      line = vase.lines();
    Command command = parse(cmd, eof);
    bool was_quitting = quitting;
    quitting = false;
    bool was_editing = editing;
    editing = false;
    switch (command.type) {
    case Command::Type::Quit:
      if (modified && !was_quitting) {
        quitting = true;
        throw ed_error("Buffer modified.");
      } else {
        return false;
      }
    case Command::Type::ForceQuit:
      return false;
    case Command::Type::None: {
      if (command.start.type != Command::Address::Type::None) {
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line);
        else
          resolve_address(command.start, &line);
      } else {
        line++;
      }
      if (line == 0)
        throw ed_error("Line 0 is invalid.");
      if (line > vase.lines())
        throw ed_error("Line position too high.");
      Iterator it = vase.iterate(line - 1, Direction::Forward);
      it.next();
      std::cout << it.line << std::endl;
    } break;
    case Command::Type::Invalid:
      throw ed_error("Invalid command.");
    case Command::Type::HelpToggle:
      help_mode = !help_mode;
      if (help_mode && last_message != "")
        std::cout << last_message << std::endl;
      break;
    case Command::Type::Help:
      if (last_message != "")
        std::cout << last_message << std::endl;
      break;
    case Command::Type::PromptToggle:
      prompt_mode = !prompt_mode;
      break;
    case Command::Type::Print: {
      uint64_t line_start = line;
      uint64_t line_end = line;
      if (command.start.type != Command::Address::Type::None) {
        resolve_address(command.start, &line_start);
        line_end = line_start;
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line_end);
      }
      if (line_start == 0)
        throw ed_error("Line 0 is invalid.");
      if (line_end < line_start)
        throw ed_error("Invalid address range.");
      Iterator it = vase.iterate(line_start - 1, Direction::Forward);
      while (it.next() && line_start++ <= line_end)
        std::cout << it.line << std::endl;
      line = line_end;
    } break;
    case Command::Type::Number: {
      uint64_t line_start = line;
      uint64_t line_end = line;
      if (command.start.type != Command::Address::Type::None) {
        resolve_address(command.start, &line_start);
        line_end = line_start;
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line_end);
      }
      if (line_start == 0)
        throw ed_error("Line 0 is invalid.");
      if (line_end < line_start)
        throw ed_error("Invalid address range.");
      Iterator it = vase.iterate(line_start - 1, Direction::Forward);
      while (it.next() && line_start <= line_end)
        std::cout << line_start++ << "\t" << it.line << std::endl;
      line = line_end;
    } break;
    case Command::Type::Append: {
      std::string text;
      std::string cline;
      uint64_t line_count = 0;
      while (std::getline(std::cin, cline)) {
        if (cline == ".")
          break;
        line_count++;
        text.append(cline);
        text.push_back('\n');
      }
      if (command.start.type != Command::Address::Type::None) {
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line);
        else
          resolve_address(command.start, &line);
      }
      if (text.empty()) {
        if (line == 0)
          line = 1;
        break;
      }
      vase.snapshot();
      append(text, line);
      modified = true;
      line += line_count;
    } break;
    case Command::Type::Insert: {
      std::string text;
      std::string cline;
      uint64_t line_count = 0;
      while (std::getline(std::cin, cline)) {
        if (cline == ".")
          break;
        line_count++;
        text.append(cline);
        text.push_back('\n');
      }
      if (command.start.type != Command::Address::Type::None) {
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line);
        else
          resolve_address(command.start, &line);
      }
      if (line == 0)
        line = 1;
      if (text.empty())
        break;
      vase.snapshot();
      append(text, line - 1);
      modified = true;
      line += line_count - 1;
    } break;
    case Command::Type::Change: {
      uint64_t line_start = line;
      uint64_t line_end = line;
      if (command.start.type != Command::Address::Type::None) {
        resolve_address(command.start, &line_start);
        if (line_start == 0)
          line_start = 1;
        line_end = line_start;
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line_end);
      }
      if (line_end < line_start)
        throw ed_error("Invalid address range.");
      std::string text;
      std::string cline;
      uint64_t line_count = 0;
      while (std::getline(std::cin, cline)) {
        if (cline == ".")
          break;
        line_count++;
        text.append(cline);
        text.push_back('\n');
      }
      vase.snapshot();
      remove(line_start, line_end);
      line = line_start;
      if (text.empty())
        break;
      append(text, line);
      modified = true;
      line += line_count;
    } break;
    case Command::Type::Delete: {
      uint64_t line_start = line;
      uint64_t line_end = line;
      if (command.start.type != Command::Address::Type::None) {
        resolve_address(command.start, &line_start);
        if (line_start == 0)
          line_start = 1;
        line_end = line_start;
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line_end);
      }
      if (line_end < line_start)
        throw ed_error("Invalid address range.");
      vase.snapshot();
      vase.erase({{line_start - 1, 0}, {line_end, 0}});
      modified = true;
      line = line_start;
      if (line > vase.lines())
        line = vase.lines();
    } break;
    case Command::Type::Write: {
      std::string path = command.argument;
      if (path.empty()) {
        path = save_path;
        if (path.empty())
          throw ed_error("Need file to write to.");
      }
      uint64_t line_start = 1;
      uint64_t line_end = vase.lines();
      if (vase.lines() == 0)
        line_start = line_end = 0;
      if (command.start.type != Command::Address::Type::None) {
        resolve_address(command.start, &line_start);
        line_end = line_start;
        if (command.address_flags & Command::RANGE)
          resolve_address(command.end, &line_end);
      }
      if (vase.lines() && line_start == 0)
        throw ed_error("Line 0 is invalid.");
      if (line_end < line_start)
        throw ed_error("Invalid address range.");
      uint64_t bytes = 0;
      if (path[0] == '!') {
        FILE *pipe = popen(path.c_str() + 1, "w");
        if (!pipe)
          throw ed_error("Error starting command.");
        Iterator it = vase.iterate(line_start - 1, Direction::Forward);
        while (it.next() && line_start++ <= line_end) {
          it.line += '\n';
          bytes += it.line.size();
          if (fputs(it.line.c_str(), pipe) == EOF) {
            pclose(pipe);
            throw ed_error("Error writing to command.");
          }
        }
        if (pclose(pipe) == -1)
          throw ed_error("Error closing command.");
      } else {
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file)
          throw ed_error("Error writing to file.");
        Iterator it = vase.iterate(line_start - 1, Direction::Forward);
        while (it.next() && line_start++ <= line_end) {
          file << it.line << '\n';
          bytes += it.line.size() + 1;
        }
        if (!file)
          throw ed_error("Error writing to file.");
        save_path = path;
        modified = false;
      }
      if (!suppress_mode)
        std::cout << bytes << std::endl;
    } break;
    case Command::Type::Edit: {
      if (modified && !was_editing) {
        editing = true;
        throw ed_error("Buffer modified.");
      }
      std::string path = command.argument;
      if (path.empty()) {
        path = save_path;
        if (path.empty())
          throw ed_error("Need file to read from.");
      }
      if (path[0] == '!') {
        path.erase(1);
        Vase new_vase(std::string(path), "/tmp");
        vase = std::move(new_vase);
        modified = true;
      } else {
        Vase new_vase(std::filesystem::path(path), "/tmp");
        vase = std::move(new_vase);
        save_path = path;
        modified = false;
      }
      line = vase.lines();
      if (!suppress_mode)
        std::cout << vase.length() << std::endl;
    } break;
    case Command::Type::ForceEdit: {
      std::string path = command.argument;
      if (path.empty()) {
        path = save_path;
        if (path.empty())
          throw ed_error("Need file to read from.");
      }
      if (path[0] == '!') {
        path.erase(0, 1);
        Vase new_vase(std::string(path), "/tmp");
        vase = std::move(new_vase);
        modified = true;
      } else {
        Vase new_vase(std::filesystem::path(path), "/tmp");
        vase = std::move(new_vase);
        save_path = path;
        modified = false;
      }
      line = vase.lines();
      std::fill(std::begin(marks), std::end(marks), '\0');
      if (!suppress_mode)
        std::cout << vase.length() << std::endl;
    } break;
    case Command::Type::Filename: {
      std::string path = command.argument;
      if (path.empty())
        throw ed_error("No filename given.");
      save_path = path;
      std::cout << save_path.string() << std::endl;
    } break;
    case Command::Type::Dump:
      Shard::dump(vase.root);
      break;
    }
    if (command.suffix) {
      Iterator it = vase.iterate(line - 1, Direction::Forward);
      it.next();
      switch (command.suffix) {
      case 'p':
        std::cout << it.line << std::endl;
        break;
      case 'n':
        std::cout << line << "\t" << it.line << std::endl;
        break;
      case 'l':
        // TODO: escaping.
        std::cout << it.line << std::endl;
        break;
      }
    }
  } catch (ed_error &e) {
    last_message = e.what();
    std::cout << "?" << std::endl;
    if (help_mode)
      std::cout << e.what() << std::endl;
  }
  return true;
}
} // namespace crib::commands::ed
