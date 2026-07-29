#include "vase/vase.h"
#include "vase/search.h"

Vase::Vase(char *data, uint32_t length) : original(data, length), append() {
  root = new Petal(length, original.newlines.size(), &original, 0);
}

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

void Vase::type(uint32_t offset, char key) {
  uint32_t pos = append.key(key);
  Shard *inserted = new Petal(1, key == '\n', &append, pos);
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

void Vase::insert(uint32_t offset, const char *data, uint32_t len) {
  uint32_t lines = 0;
  uint32_t pos = append.append(data, len, &lines);
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

void Vase::erase(uint32_t cursor, int64_t amount) {
  if (amount == 0)
    return;
  uint32_t start;
  uint32_t count;
  if (amount < 0) {
    count = std::min<uint32_t>(-amount, cursor);
    start = cursor - count;
  } else {
    start = cursor;
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
      uint32_t pos = buf.append(constant.data(), (uint32_t)constant.size(), &lines);
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

Shard *build_balanced(Shard **pieces, size_t lo, size_t hi) {
  if (hi - lo == 1)
    return pieces[lo];
  size_t mid = lo + (hi - lo) / 2;
  Shard *left = build_balanced(pieces, lo, mid);
  Shard *right = build_balanced(pieces, mid, hi);
  Shard *node = new Branch(left, right);
  Shard::release(left);
  Shard::release(right);
  return node;
}

void Vase::regex_search_replace(
  std::string_view pattern,
  uint32_t start_offset, uint32_t end_offset,
  std::string_view replace, std::string_view options
) {
  std::vector<RegexMatch> matches = regex_search(root, pattern, start_offset, end_offset, options);
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
