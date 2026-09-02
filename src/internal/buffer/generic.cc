#include "bed.h"
#include "internal/buffer/buffer.h"

namespace bed::internal::buffer {
GenericBuffer::~GenericBuffer() {
  for (auto &item : undo_stack) {
    syntax::release(item.parse_state);
    vase::Shard::release(item.text);
  }
  for (auto &item : redo_stack) {
    syntax::release(item.parse_state);
    vase::Shard::release(item.text);
  }
}

void GenericBuffer::list_history(BEd &ctx) {
  uint64_t current = base_version + undo_stack.size();
  for (size_t i = 0; i < undo_stack.size(); ++i) {
    auto &item = undo_stack[i];
    uint64_t version = base_version + i;
    auto time = std::chrono::system_clock::to_time_t(item.timestamp);
    std::tm tm = *std::localtime(&time);
    ctx.io.write_line(
      std::format(
        "  {}  {:04}-{:02}-{:02} {:02}:{:02}:{:02} {}",
        version,
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        item.summary
      )
    );
  }
  {
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm = *std::localtime(&time);
    ctx.io.write_line(
      std::format(
        "X {}  {:04}-{:02}-{:02} {:02}:{:02}:{:02} {}",
        current,
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        action
      )
    );
  }
  for (size_t i = 0; i < redo_stack.size(); ++i) {
    auto &item = redo_stack[redo_stack.size() - 1 - i];
    uint64_t version = current + i + 1;
    auto time = std::chrono::system_clock::to_time_t(item.timestamp);
    std::tm tm = *std::localtime(&time);
    ctx.io.write_line(
      std::format(
        "  {}  {:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        version,
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        item.summary
      )
    );
  }
}

HistoryBuffer *GenericBuffer::get_history(uint64_t version) {
  uint64_t current = base_version + undo_stack.size();
  if (version < base_version)
    throw ed_error("History version has been pruned.");
  if (version < current) {
    auto &item = undo_stack[version - base_version];
    return new HistoryBuffer(name, item.text, item.parse_state);
  }
  if (version == current)
    return new HistoryBuffer(name, root, parse);
  uint64_t redo_offset = version - current - 1;
  if (redo_offset >= redo_stack.size())
    throw ed_error("No such history version.");
  auto &item = redo_stack[redo_stack.size() - 1 - redo_offset];
  return new HistoryBuffer(name, item.text, item.parse_state);
}

void GenericBuffer::snapshot(std::string action_) {
  vase::Shard::retain(root);
  undo_stack.push_back(
    HistoryItem{
      syntax::retain(parse),
      root,
      timestamp,
      action
    }
  );
  for (auto &item : redo_stack) {
    syntax::release(item.parse_state);
    vase::Shard::release(item.text);
  }
  redo_stack.clear();
  timestamp = std::chrono::system_clock::now();
  action = action_;
}

bool GenericBuffer::undo(BEd &ctx) {
  if (undo_stack.empty())
    return false;
  HistoryItem prev = undo_stack.back();
  undo_stack.pop_back();
  vase::Shard::retain(root);
  redo_stack.push_back(
    HistoryItem{
      syntax::retain(parse),
      root,
      timestamp,
      action
    }
  );
  vase::Shard::release(root);
  root = prev.text;
  syntax::release(parse);
  parse = prev.parse_state;
  state = Modified;
  if (!root) {
    ctx.prev().buffername = name;
    ctx.prev().start = 0;
    ctx.prev().end = 0;
  } else {
    ctx.prev().buffername = name;
    ctx.prev().start = 1;
    ctx.prev().end = root->lines + 1;
  }
  return true;
}

bool GenericBuffer::redo(BEd &ctx) {
  if (redo_stack.empty())
    return false;
  HistoryItem next = redo_stack.back();
  redo_stack.pop_back();
  vase::Shard::retain(root);
  undo_stack.push_back(
    HistoryItem{
      syntax::retain(parse),
      root,
      timestamp,
      action
    }
  );
  vase::Shard::release(root);
  root = next.text;
  syntax::release(parse);
  parse = next.parse_state;
  state = Modified;
  if (!root) {
    ctx.prev().buffername = name;
    ctx.prev().start = 0;
    ctx.prev().end = 0;
  } else {
    ctx.prev().buffername = name;
    ctx.prev().start = 1;
    ctx.prev().end = root->lines + 1;
  }
  return true;
}

void GenericBuffer::prune(int keep) {
  size_t drop =
    undo_stack.size() > (size_t)keep
      ? undo_stack.size() - keep
      : 0;
  for (size_t i = 0; i < drop; ++i) {
    syntax::release(undo_stack[i].parse_state);
    vase::Shard::release(undo_stack[i].text);
  }
  undo_stack.erase(
    undo_stack.begin(),
    undo_stack.begin() + drop
  );
  base_version += drop;
  for (auto &item : redo_stack) {
    syntax::release(item.parse_state);
    vase::Shard::release(item.text);
  }
  redo_stack.clear();
}

bool GenericBuffer::waste() {
  return save_path.empty()
         && root == nullptr;
}

void GenericBuffer::load(BEd &ctx, vase::Shard *text) {
  snapshot("Load file.");
  if (lines())
    ctx.marks.erase(name, 1, lines());
  vase::Shard::release(root);
  vase::Shard::retain(text);
  root = text;
  state = buffer::GenericBuffer::Unmodified;
  if (!text) {
    ctx.prev().buffername = name;
    ctx.prev().start = 0;
    ctx.prev().end = 0;
  } else {
    ctx.prev().buffername = name;
    ctx.prev().start = 1;
    ctx.prev().end = text->lines + 1;
  }
  syntax::release(parse);
  parse = syntax::make_parser(root, lines(), ctx.languages["ruby"]);
}

void GenericBuffer::set_filename(std::filesystem::path path) {
  save_path = path;
}

std::filesystem::path GenericBuffer::filename() {
  return save_path;
};

void GenericBuffer::append(BEd &ctx, vase::Shard *text, uint64_t line) {
  if (!text)
    return;
  snapshot(std::format("Insert {} lines after line {}", text->lines + 1, line));
  ctx.prev().buffername = name;
  ctx.prev().start = line + 1;
  ctx.prev().end = line + text->lines + 1;
  root = vase::insert(&ctx.append, root, text, line);
  ctx.marks.insert(name, line, text->lines + 1);
  if (parse.lang)
    syntax::insert(parse, root, line, text->lines + 1);
  state = Modified;
}

void GenericBuffer::remove(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  snapshot(std::format("Remove lines {} to {}", start_line, end_line));
  root = vase::erase(root, start_line, end_line);
  ctx.prev().buffername = name;
  ctx.prev().start = std::min(start_line, lines());
  ctx.prev().end = std::min(start_line, lines());
  ctx.marks.erase(name, start_line, end_line - start_line + 1);
  if (parse.lang)
    syntax::erase(parse, root, start_line, end_line - start_line + 1);
  state = Modified;
}

void GenericBuffer::replace(BEd &ctx, vase::Shard *text, uint64_t start_line, uint64_t end_line) {
  if (!text) {
    remove(ctx, start_line, end_line);
    return;
  }
  snapshot(std::format("Replace lines {} to {} with {} lines", start_line, end_line, text->lines));
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = start_line + text->lines;
  uint64_t new_count = text->lines + 1;
  uint64_t old_count = end_line - start_line + 1;
  root = vase::replace(root, text, start_line, end_line);
  if (parse.lang) {
    syntax::Edit edit(parse);
    edit.erase(start_line, old_count);
    edit.insert(start_line, new_count);
    edit.commit(root);
  }
  if (new_count > old_count) {
    uint64_t diff = new_count - old_count;
    ctx.marks.insert(name, end_line, diff);
  } else if (new_count < old_count) {
    uint64_t diff = old_count - new_count;
    ctx.marks.collapse(name, start_line + new_count - 1, diff);
  }
  state = Modified;
}

void GenericBuffer::join(BEd &ctx, uint64_t start_line, uint64_t end_line) {
  snapshot(std::format("Join lines {} to {}", start_line, end_line));
  root = vase::join(root, start_line, end_line);
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = start_line;
  ctx.marks.collapse(name, start_line, end_line - start_line);
  if (parse.lang)
    syntax::erase(parse, root, start_line, end_line - start_line);
  state = Modified;
}

void GenericBuffer::substitute(
  BEd &ctx, uint64_t start_line, uint64_t end_line,
  std::string &regex, std::string &replacement, std::string &options
) {
  snapshot(std::format("Substitute /{}/ with /{}/ in lines {} to {}", regex, replacement, start_line, end_line));
  ctx.prev().buffername = name;
  ctx.prev().start = start_line;
  ctx.prev().end = end_line;
  std::optional<syntax::Edit> edit;
  if (parse.lang)
    edit.emplace(parse);
  root = vase::substitute(
    &ctx.append,
    root,
    regex,
    start_line,
    end_line,
    replacement,
    options,
    [&](uint64_t line, uint64_t old_lines, uint64_t new_lines) {
      if (old_lines) {
        ctx.marks.erase(name, line, old_lines);
        if (edit)
          edit->erase(line, old_lines);
      }
      if (new_lines) {
        ctx.marks.insert(name, line, new_lines);
        if (edit)
          edit->insert(line, new_lines);
      }
    }
  );
  if (edit)
    edit->commit(root);
  ctx.prev().buffername = name;
  state = Modified;
}
} // namespace bed::internal::buffer
