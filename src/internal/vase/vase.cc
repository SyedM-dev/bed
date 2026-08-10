#include "internal/vase/vase.h"

namespace crib::internal::vase {
Vase::Vase(std::filesystem::path path, std::filesystem::path swapdir)
    : path(path), swapdir(swapdir) {
  if (!std::filesystem::exists(swapdir) || !std::filesystem::is_directory(swapdir))
    throw std::runtime_error("Swap directory does not exist or is not a directory.");
  append = new AppendBuffer(swapdir);
  original = new OriginalBuffer(swapdir);
  if (std::filesystem::is_regular_file(path))
    root = Shard::from_file(path, original, posix_ending);
  else
    root = nullptr;
  history_top = 0;
  history.push_back(root);
  Shard::retain(root);
}

Vase::Vase(std::string cmd, std::filesystem::path swapdir)
    : path(""), swapdir(swapdir) {
  if (!std::filesystem::exists(swapdir) || !std::filesystem::is_directory(swapdir))
    throw std::runtime_error("Swap directory does not exist or is not a directory.");
  append = new AppendBuffer(swapdir);
  original = new OriginalBuffer(swapdir);
  root = Shard::from_command(cmd.c_str(), original, posix_ending);
  history_top = 0;
  history.push_back(root);
  Shard::retain(root);
}

Vase::Vase(std::filesystem::path swapdir)
    : path(""), swapdir(swapdir) {
  if (!std::filesystem::exists(swapdir) || !std::filesystem::is_directory(swapdir))
    throw std::runtime_error("Swap directory does not exist or is not a directory.");
  append = new AppendBuffer(swapdir);
  original = new OriginalBuffer(swapdir);
  root = nullptr;
  history_top = 0;
  history.push_back(root);
  Shard::retain(root);
}

Vase::~Vase() {
  Shard::release(root);
  for (auto s : history)
    Shard::release(s);
  if (original)
    delete original;
  if (append)
    delete append;
}

Vase::Vase(Vase &&other) noexcept
    : original(other.original),
      append(other.append),
      root(other.root),
      posix_ending(other.posix_ending),
      using_crlf(other.using_crlf),
      path(std::move(other.path)),
      swapdir(std::move(other.swapdir)),
      history(std::move(other.history)),
      history_top(other.history_top) {
  other.original = nullptr;
  other.append = nullptr;
  other.root = nullptr;
  other.history_top = 0;
}

Vase &Vase::operator=(Vase &&other) noexcept {
  if (this == &other)
    return *this;
  Shard::release(root);
  for (auto s : history)
    Shard::release(s);
  delete original;
  delete append;
  original = other.original;
  append = other.append;
  root = other.root;
  posix_ending = other.posix_ending;
  using_crlf = other.using_crlf;
  path = std::move(other.path);
  swapdir = std::move(other.swapdir);
  history = std::move(other.history);
  history_top = other.history_top;
  other.original = nullptr;
  other.append = nullptr;
  other.root = nullptr;
  other.history_top = 0;
  return *this;
}

uint64_t Vase::length() {
  if (!root)
    return 0;
  return root->length + posix_ending;
}

uint64_t Vase::lines() {
  if (!root)
    return 0;
  return root->lines + 1;
}

std::string Vase::to_string() {
  std::string out;
  if (!root)
    return out;
  PetalIterator it(root, Direction::Forward);
  it.seek_offset(0);
  const char *data;
  uint64_t len;
  while (it.next(&data, &len))
    out.append(data, len);
  if (posix_ending)
    out.append("\n");
  return out;
}

std::string Vase::to_string(Range range) {
  clamp(&range.start);
  clamp(&range.end);
  std::string out;
  if (!root)
    return out;
  PetalIterator it(root, Direction::Forward);
  uint64_t start = offset_of(range.start);
  it.seek_offset(start);
  const char *data;
  uint64_t len;
  uint64_t remaining = offset_of(range.end) - start;
  while (remaining && it.next(&data, &len)) {
    uint64_t n = std::min(len, remaining);
    out.append(data, n);
    remaining -= n;
  }
  return out;
}

Iterator Vase::iterate(uint64_t line, Direction dir) {
  return Iterator(root, line, dir);
}

bool Vase::undo() {
  if (history_top == 0)
    return false;
  Shard::release(root);
  history_top--;
  root = history[history_top];
  Shard::retain(root);
  return true;
}

bool Vase::redo() {
  if (history_top + 1 >= history.size())
    return false;
  Shard::release(root);
  history_top++;
  root = history[history_top];
  Shard::retain(root);
  return true;
}

void Vase::snapshot() {
  if (history[history_top] == root)
    return;
  while (history.size() > history_top + 1) {
    Shard::release(history.back());
    history.pop_back();
  }
  Shard::retain(root);
  history.push_back(root);
  history_top++;
}

void Vase::prune_history(uint64_t n) {
  uint64_t keep = std::min(history.size(), n + 1);
  if (keep == history.size())
    return;
  uint64_t remove = history.size() - keep;
  for (uint64_t i = 0; i < remove; ++i)
    Shard::release(history[i]);
  history.erase(history.begin(), history.begin() + remove);
  history_top -= remove;
}

bool Vase::save() {
  if (!root)
    return true;
  if (path == "")
    return false;
  std::ofstream file(path, std::ios::binary);
  if (!file)
    return false;
  PetalIterator it(root, Direction::Forward);
  it.seek_offset(0);
  const char *data;
  uint64_t len;
  while (it.next(&data, &len)) {
    file.write(data, len);
    if (!file)
      return false;
  }
  if (posix_ending)
    file.write("\n", 1);
  if (!file)
    return false;
  return true;
}

bool Vase::save_swap() {
  return false;
}

void Vase::insert(Point *point, char key) {
  uint64_t pos = append->append(key);
  Shard *inserted = new Petal(1, key == '\n', append, pos);
  auto [left, right] = Shard::split(root, offset_of(*point));
  Shard *left2 = Shard::append(left, inserted);
  Shard::release(left);
  Shard::release(inserted);
  Shard *new_root = Shard::concat(left2, right);
  Shard::release(left2);
  Shard::release(right);
  Shard::release(root);
  root = new_root;
  if (key == '\n')
    *point = {point->row + 1, 0};
  else
    point->col++;
}

void Vase::insert(Point *point, std::string_view str) {
  insert(point, str.data(), str.size());
}

void Vase::insert(Point *point, const char *data, uint64_t len) {
  while (len) {
    uint64_t chunk_size = std::min<uint64_t>(len, PETAL_SIZE_MAX);
    _insert(point, data, chunk_size);
    len -= chunk_size;
    data += chunk_size;
  }
}

void Vase::_insert(Point *point, const char *data, uint64_t len) {
  if (len == 0)
    return;
  uint64_t offset = offset_of(*point);
  uint64_t pos = append->append(data, len);
  uint64_t lines = 0;
  const char *start = data;
  const char *last_line = start;
  const char *end = start + len;
  while ((data = (const char *)memchr(data, '\n', end - data))) {
    ++lines;
    last_line = ++data;
  }
  uint64_t col = 0;
  uint64_t remaining = end - last_line;
  while (remaining) {
    uint64_t n = grapheme_next_character_break_utf8(last_line, remaining);
    last_line += n;
    remaining -= n;
    ++col;
  }
  if (lines) {
    point->row += lines;
    point->col = col;
  } else {
    point->col += col;
  }
  Shard *inserted = new Petal(len, lines, append, pos);
  auto [left, right] = Shard::split(root, offset);
  Shard *left2 = Shard::append(left, inserted);
  Shard::release(left);
  Shard::release(inserted);
  Shard *new_root = Shard::concat(left2, right);
  Shard::release(left2);
  Shard::release(right);
  Shard::release(root);
  root = new_root;
}

void Vase::erase(Point *point, uint64_t amount, Direction dir) {
  if (amount == 0)
    return;
  Point start = *point;
  Point end = *point;
  if (dir == Direction::Forward)
    move_clusters(&end, amount, Direction::Forward);
  else
    move_clusters(&start, amount, Direction::Backward);
  uint64_t start_offset = offset_of(start);
  uint64_t end_offset = offset_of(end);
  if (start_offset > end_offset)
    std::swap(start_offset, end_offset);
  uint64_t count = end_offset - start_offset;
  auto [a, b] = Shard::split(root, start_offset);
  auto [d, c] = Shard::split(b, count);
  Shard *new_root = Shard::concat(a, c);
  Shard::release(a);
  Shard::release(b);
  Shard::release(c);
  Shard::release(d);
  Shard::release(root);
  root = new_root;
  if (dir == Direction::Backward)
    *point = start;
}

void Vase::erase(Range range) {
  Point start = range.start;
  Point end = range.end;
  uint64_t start_offset = offset_of(start);
  uint64_t end_offset = offset_of(end);
  uint64_t count = end_offset - start_offset;
  auto [a, b] = Shard::split(root, start_offset);
  auto [d, c] = Shard::split(b, count);
  Shard *new_root = Shard::concat(a, c);
  Shard::release(a);
  Shard::release(b);
  Shard::release(c);
  Shard::release(d);
  Shard::release(root);
  root = new_root;
}

void Vase::replace(Range range, std::string_view str) {
  replace(range, str.data(), str.size());
}

void Vase::replace(Range range, const char *data, uint64_t len) {
  erase(range);
  insert(&range.start, data, len);
}

uint64_t Vase::offset_of(Point point) {
  clamp(&point);
  LineIterator it(root, point.row, Direction::Forward);
  std::string line;
  uint64_t offset = 0;
  if (it.next(&line)) {
    const char *ptr = line.data();
    uint64_t remaining = line.length();
    while (point.col && remaining) {
      uint64_t next_len = grapheme_next_character_break_utf8(ptr, remaining);
      remaining -= next_len;
      ptr += next_len;
      offset += next_len;
      point.col--;
    }
  }
  return it.byte_offset() + offset;
}

Point Vase::point_of(uint64_t offset) {
  PetalIterator it(root, Direction::Backward);
  it.seek_offset(offset);
  Point p;
  p.row = it.global_line;
  std::string line;
  const char *chunk;
  uint64_t len = 0;
  if (!it.next(&chunk, &len))
    return p;
  while (true) {
#if defined(__GLIBC__) || defined(__APPLE__)
    const char *nl = (const char *)memrchr(chunk, '\n', len);
#else
    const char *nl = nullptr;
    const char *p = chunk + len;
    while (!nl && p != chunk)
      if (*(--p) == '\n')
        nl = p;
#endif
    if (!nl) {
      if (len && chunk[len - 1] == '\r')
        --len;
      line.insert(0, chunk, len);
      if (!it.next(&chunk, &len))
        break;
      continue;
    }
    const char *end = chunk + len;
    const char *start = nl + 1;
    const char *line_end = end;
    if (line_end > start && *(line_end - 1) == '\r')
      --line_end;
    line.insert(0, start, line_end - start);
    break;
  }
  const char *ptr = line.data();
  uint64_t remaining = line.length();
  while (remaining) {
    uint64_t next_len = grapheme_next_character_break_utf8(ptr, remaining);
    remaining -= next_len;
    ptr += next_len;
    p.col++;
  }
  clamp(&p);
  return p;
}

void Vase::move_clusters(Point *point, uint64_t amount, Direction dir) {
  if (amount == 0)
    return;
  if (dir == Direction::Backward) {
    LineIterator it(root, point->row, Direction::Backward);
    while (amount) {
      std::string line;
      if (!it.next(&line))
        return;
      std::vector<uint64_t> clusters;
      const char *ptr = line.data();
      uint64_t remaining = line.size();
      uint64_t byte = 0;
      while (remaining) {
        clusters.push_back(byte);
        uint64_t len =
          grapheme_next_character_break_utf8(ptr, remaining);
        ptr += len;
        remaining -= len;
        byte += len;
      }
      while (amount && point->col) {
        point->col--;
        amount--;
      }
      if (amount == 0)
        return;
      if (point->row == 0)
        return;
      point->row--;
      point->col = clusters.size();
      amount--;
    }
  } else {
    LineIterator it(root, point->row, Direction::Forward);
    while (amount) {
      std::string line;
      if (!it.next(&line))
        return;
      if (point->col || amount) {
        const char *ptr = line.data();
        uint64_t remaining = line.size();
        uint64_t col = 0;
        while (col < point->col && remaining) {
          uint64_t len = grapheme_next_character_break_utf8(ptr, remaining);
          ptr += len;
          remaining -= len;
          col++;
        }
        while (amount && remaining) {
          uint64_t len = grapheme_next_character_break_utf8(ptr, remaining);
          ptr += len;
          remaining -= len;
          point->col++;
          amount--;
        }
      }
      if (amount) {
        point->row++;
        point->col = 0;
        amount--;
      }
    }
  }
  clamp(point);
}

void Vase::clamp(Point *point) {
  if (!root) {
    point->row = 0;
    point->col = 0;
    return;
  }
  if (point->row > root->lines) {
    point->row = root->lines;
    point->col = UINT64_MAX;
  }
  LineIterator it(root, point->row, Direction::Forward);
  std::string line;
  uint64_t clusters = 0;
  if (it.next(&line)) {
    const char *ptr = line.data();
    uint64_t remaining = line.length();
    while (remaining) {
      uint64_t next_len = grapheme_next_character_break_utf8(ptr, remaining);
      remaining -= next_len;
      ptr += next_len;
      clusters++;
    }
  }
  if (point->col > clusters)
    point->col = clusters;
}

void Vase::move_lines(Point *point, uint64_t amount, Direction dir) {
  if (dir == Direction::Forward) {
    point->row += amount;
  } else {
    if (amount > point->row)
      point->row = 0;
    else
      point->row -= amount;
  }
  clamp(point);
}
} // namespace crib::internal::vase
