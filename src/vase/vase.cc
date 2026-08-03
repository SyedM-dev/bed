#include "vase/vase.h"
#include "utils/utils.h"
#include "vase/iterators/line.h"
#include "vase/search.h"

Vase::Vase(std::string path)
    : original(path), append(),
      root(create_shards(&original)) {
  history.push_back(root);
  Shard::retain(root);
  history_top = 0;
}

Vase::~Vase() {
  Shard::release(root);
  for (auto s : history)
    Shard::release(s);
}

uint32_t Vase::length() {
  return root->length;
}

std::string Vase::to_string() {
  std::string out;
  flatten(root, out);
  return out;
}

LineIterator Vase::iterate(uint32_t line) {
  return LineIterator(root, line, Direction::Forward);
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

void Vase::prune_history(uint32_t n) {
  n = std::min(n, history_top);
  for (uint32_t i = 0; i < n; ++i)
    Shard::release(history[i]);
  history.erase(history.begin(), history.begin() + n);
  history_top -= n;
}

void Vase::insert(Point *point, char key) {
  if (key == '\n')
    *point = {point->row + 1, 0};
  else
    point->col++;
  uint32_t pos = append.key(key);
  Shard *inserted = new Petal(1, key == '\n', &append, pos);
  auto [left, right] = split_shard(root, offset_of(*point));
  Shard *left2 = append_leaf(left, inserted);
  Shard *new_root = concat_shard(left2, right);
  Shard::release(left);
  Shard::release(right);
  Shard::release(left2);
  Shard::release(inserted);
  Shard::release(root);
  root = new_root;
}

void Vase::insert(Point *point, const char *data, uint32_t len) {
  while (len) {
    uint32_t chunk_size = std::min<uint32_t>(len, PETAL_SIZE_MAX);
    _insert(point, data, chunk_size);
    len -= chunk_size;
    data += chunk_size;
  }
}

void Vase::_insert(Point *point, const char *data, uint32_t len) {
  if (len == 0)
    return;
  uint32_t offset = offset_of(*point);
  uint32_t pos = append.append(data, len);
  uint32_t lines = 0;
  const char *start = data;
  const char *last_line = start;
  const char *end = start + len;
  while ((data = (const char *)memchr(data, '\n', end - data))) {
    ++lines;
    last_line = ++data;
  }
  uint32_t col = 0;
  uint32_t remaining = end - last_line;
  while (remaining) {
    uint32_t n = grapheme_next_character_break_utf8(last_line, remaining);
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
  Shard *inserted = new Petal(len, lines, &append, pos);
  auto [left, right] = split_shard(root, offset);
  Shard *left2 = append_leaf(left, inserted);
  Shard *new_root = concat_shard(left2, right);
  Shard::release(left);
  Shard::release(right);
  Shard::release(left2);
  Shard::release(inserted);
  Shard::release(root);
  root = new_root;
}

void Vase::erase(Point *point, int64_t amount) {
  if (amount == 0)
    return;
  Point start = *point;
  Point end = *point;
  if (amount < 0)
    move_clusters(&start, amount);
  else
    move_clusters(&end, amount);
  uint32_t start_offset = offset_of(start);
  uint32_t end_offset = offset_of(end);
  if (start_offset > end_offset)
    std::swap(start_offset, end_offset);
  uint32_t count = end_offset - start_offset;
  auto [a, b] = split_shard(root, start_offset);
  auto [d, c] = split_shard(b, count);
  Shard *new_root = concat_shard(a, c);
  Shard::release(a);
  Shard::release(b);
  Shard::release(c);
  Shard::release(d);
  Shard::release(root);
  root = new_root;
  if (amount < 0)
    *point = start;
}

void Vase::erase(Range range) {
  Point start = range.start;
  Point end = range.end;
  uint32_t start_offset = offset_of(start);
  uint32_t end_offset = offset_of(end);
  uint32_t count = end_offset - start_offset;
  auto [a, b] = split_shard(root, start_offset);
  auto [d, c] = split_shard(b, count);
  Shard *new_root = concat_shard(a, c);
  Shard::release(a);
  Shard::release(b);
  Shard::release(c);
  Shard::release(d);
  Shard::release(root);
  root = new_root;
}

void Vase::replace(Range range, const char *data, uint32_t len) {
  erase(range);
  insert(&range.start, data, len);
}

void Vase::flatten(Shard *s, std::string &out) {
  if (s->kind == Shard::ShardKind::Petal) {
    auto *p = (Petal *)s;
    uint32_t remaining = p->length;
    uint32_t pos = p->pos;
    while (remaining) {
      uint32_t got;
      const char *data = p->source->read(pos, &got);
      uint32_t take = std::min(got, remaining);
      out.append(data, take);
      remaining -= take;
      pos += take;
    }
  } else {
    auto *b = (Branch *)s;
    flatten(b->left, out);
    flatten(b->right, out);
  }
}

uint32_t Vase::offset_of(Point point) {
  LineIterator it(root, point.row, Direction::Forward);
  std::string line;
  uint32_t offset = 0;
  if (it.next(line)) {
    const char *ptr = line.data();
    uint32_t remaining = line.length();
    while (point.col && remaining) {
      uint32_t next_len = grapheme_next_character_break_utf8(ptr, remaining);
      remaining -= next_len;
      ptr += next_len;
      offset += next_len;
      point.col--;
    }
  }
  return it.byte_offset() + offset;
}

void Vase::move_clusters(Point *point, int64_t amount) {
  if (amount == 0)
    return;
  if (amount < 0) {
    amount = -amount;
    LineIterator it(root, point->row, Direction::Backward);
    while (amount > 0) {
      std::string line;
      if (!it.next(line))
        return;
      std::vector<uint32_t> clusters;
      const char *ptr = line.data();
      uint32_t remaining = line.size();
      uint32_t byte = 0;
      while (remaining) {
        clusters.push_back(byte);
        uint32_t len =
          grapheme_next_character_break_utf8(ptr, remaining);
        ptr += len;
        remaining -= len;
        byte += len;
      }
      while (amount > 0 && point->col > 0) {
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
    while (amount > 0) {
      std::string line;
      if (!it.next(line))
        return;
      if (point->col > 0 || amount > 0) {
        const char *ptr = line.data();
        uint32_t remaining = line.size();
        uint32_t col = 0;
        while (col < point->col && remaining) {
          uint32_t len = grapheme_next_character_break_utf8(ptr, remaining);
          ptr += len;
          remaining -= len;
          col++;
        }
        while (amount > 0 && remaining) {
          uint32_t len = grapheme_next_character_break_utf8(ptr, remaining);
          ptr += len;
          remaining -= len;
          point->col++;
          amount--;
        }
      }
      if (amount > 0) {
        point->row++;
        point->col = 0;
        amount--;
      }
    }
  }
}

void Vase::clamp(Point *point) {
  if (point->row > root->lines)
    point->row = root->lines;
  LineIterator it(root, point->row, Direction::Forward);
  std::string line;
  if (!it.next(line)) {
    point->col = 0;
    return;
  }
  uint32_t clusters = 0;
  const char *ptr = line.data();
  uint32_t remaining = line.size();
  while (remaining) {
    uint32_t len = grapheme_next_character_break_utf8(ptr, remaining);
    ptr += len;
    remaining -= len;
    clusters++;
  }
  if (point->col > clusters)
    point->col = clusters;
}

void Vase::move_lines(Point *point, int64_t amount) {
  if (point->row + amount < 0)
    return;
  point->row += amount;
  clamp(point);
}

struct ReplacePart {
  enum struct PartType {
    FullMatch,
    CaptureGroup,
    Constant
  } type;

  std::variant<uint8_t, Shard *> value;
};

std::vector<ReplacePart> parse_replace(AppendBuffer &buf, std::string_view s) {
  std::vector<ReplacePart> parts;
  std::string constant;
  auto flush_constant = [&]() {
    if (!constant.empty()) {
      uint32_t lines = 0;
      uint32_t pos = buf.append(constant.data(), (uint32_t)constant.size());
      parts.push_back(
        ReplacePart{
          .type = ReplacePart::PartType::Constant,
          .value = new Petal((uint32_t)constant.size(), lines, &buf, pos)
        }
      );
      constant.clear();
    }
  };
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '\\' && i + 1 < s.size() && s[i + 1] == '$') {
      constant.push_back('$');
      ++i;
      continue;
    }
    if (c == '$' && i + 1 < s.size()) {
      char next = s[i + 1];
      if (next == '0') {
        flush_constant();
        parts.push_back(
          ReplacePart{
            .type = ReplacePart::PartType::FullMatch,
            .value = (uint8_t)0
          }
        );
        ++i;
        continue;
      }
      if (next >= '1' && next <= '9') {
        flush_constant();
        parts.push_back(
          ReplacePart{
            .type = ReplacePart::PartType::CaptureGroup,
            .value = (uint8_t)(next - '0')
          }
        );
        ++i;
        continue;
      }
    }
    constant.push_back(c);
  }
  flush_constant();
  return parts;
}

void Vase::regex_search_replace(
  std::string_view pattern,
  Point start, Point end,
  std::string_view replace, std::string_view options
) {
  std::vector<RegexMatch> matches = regex_search(root, pattern, offset_of(start), offset_of(end), options);
  if (matches.empty())
    return;

  std::vector<ReplacePart> replace_parts = parse_replace(append, replace);

  std::vector<Shard *> pieces;
  pieces.reserve(matches.size() * 2 + 1);

  Shard *remaining = root;
  Shard::retain(remaining);
  uint32_t cursor = 0;

  for (const RegexMatch &match : matches) {
    uint32_t gap = match.start - cursor;
    if (gap > 0) {
      auto [keep, rest] = split_shard(remaining, gap);
      Shard::release(remaining);
      pieces.push_back(keep);
      remaining = rest;
    }

    auto [dropped, rest2] = split_shard(remaining, match.end - match.start);
    Shard::release(remaining);
    remaining = rest2;

    for (size_t i = 0; i < replace_parts.size(); ++i) {
      const ReplacePart &part = replace_parts[i];
      switch (part.type) {
      case ReplacePart::PartType::Constant:
        Shard::retain(std::get<Shard *>(part.value));
        pieces.push_back(std::get<Shard *>(part.value));
        break;
      case ReplacePart::PartType::FullMatch:
        Shard::retain(dropped);
        pieces.push_back(dropped);
        break;
      case ReplacePart::PartType::CaptureGroup: {
        uint8_t idx = std::get<uint8_t>(part.value);
        if (idx <= 9) {
          const RegexGroup &group = match.groups[idx - 1];
          if (group.start != UINT32_MAX) {
            uint32_t ls = group.start - match.start;
            uint32_t le = group.end - match.start;
            auto [a, b] = split_shard(dropped, ls);
            auto [g, c] = split_shard(b, le - ls);
            Shard::release(a);
            Shard::release(b);
            Shard::release(c);
            pieces.push_back(g);
          }
        }
        break;
      }
      }
    }
    Shard::release(dropped);
    cursor = match.end;
  }
  pieces.push_back(remaining);

  for (auto &part : replace_parts)
    if (part.type == ReplacePart::PartType::Constant)
      Shard::release(std::get<Shard *>(part.value));

  std::vector<Shard *> compact;
  compact.reserve(pieces.size());
  for (Shard *p : pieces) {
    if (p && p->length > 0)
      compact.push_back(p);
    else if (p)
      Shard::release(p);
  }

  Shard *new_root = compact.empty() ? nullptr : build_balanced(compact.data(), 0, compact.size());
  Shard::release(root);
  root = new_root;
}
