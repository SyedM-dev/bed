#include "vase/vase.h"
#include "utils/utils.h"
#include "vase/iterators/line.h"
#include "vase/search.h"

Vase::Vase(char *data, uint32_t length)
    : original(data, length), append(),
      root(create_shards(&original)) {}

Vase::~Vase() {
  Shard::release(root);
}

uint32_t Vase::length() {
  return root->length;
}

std::string Vase::to_string() {
  std::string out;
  flatten(root, out);
  return out;
}

void Vase::input(Point *point, char key) {
  if (key == '\n')
    *point = {point->row + 1, 0};
  else
    point->col++;
  uint32_t pos = append.key(key);
  // limit petal size to 32KiB here later.
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
  // wrong: havta make amount be in clusters not bytes
  uint32_t offset = offset_of(*point);
  if (amount == 0)
    return;
  uint32_t start;
  uint32_t count;
  if (amount < 0) {
    count = std::min<uint32_t>(-amount, offset);
    start = offset - count;
  } else {
    start = offset;
    count = amount;
  }
  auto [a, b] = split_shard(root, start);
  auto [d, c] = split_shard(b, count);
  Shard *new_root = concat_shard(a, c);
  Shard::release(a);
  Shard::release(b);
  Shard::release(c);
  Shard::release(d);
  Shard::release(root);
  root = new_root;
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
  LineIterator it(root, point.row);
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

bool Vase::jump(Point *, int64_t) {
  // todo.
  return false;
}

bool Vase::clamp(Point *) {
  // todo.
  return false;
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
