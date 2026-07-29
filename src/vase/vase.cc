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

  std::variant<uint8_t, std::string> value;
};

std::vector<ReplacePart> parse_replace(std::string_view s) {
  std::vector<ReplacePart> parts;
  std::string constant;
  auto flush_constant = [&]() {
    if (!constant.empty()) {
      parts.push_back(
        ReplacePart{
          .type = ReplacePart::PartType::Constant,
          .value = std::move(constant)
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

static Shard *extract_range(Shard *tree, uint32_t start, uint32_t end) {
  if (end <= start)
    return nullptr;
  auto [left, rest] = split_shard(tree, start);
  auto [mid, right] = split_shard(rest, end - start);
  Shard::release(left);
  Shard::release(rest);
  Shard::release(right);
  return mid;
}

static void append_piece(Shard *&accum, Shard *piece) {
  if (!piece)
    return;
  if (!accum) {
    accum = piece;
    return;
  }
  Shard *combined = concat_shard(accum, piece);
  Shard::release(accum);
  Shard::release(piece);
  accum = combined;
}

void Vase::regex_search_replace(
  std::string_view pattern,
  uint32_t start_offset, uint32_t end_offset,
  std::string_view replace, std::string_view options
) {
  const std::vector<RegexMatch> matches = regex_search(root, pattern, start_offset, end_offset, options);
  if (matches.empty())
    return;

  std::vector<ReplacePart> replace_parts = parse_replace(replace);

  struct ConstantRef {
    uint32_t pos = 0;
    uint32_t lines = 0;
  };
  std::vector<ConstantRef> constants(replace_parts.size());
  for (size_t i = 0; i < replace_parts.size(); ++i) {
    if (replace_parts[i].type != ReplacePart::PartType::Constant)
      continue;
    const std::string &text = std::get<std::string>(replace_parts[i].value);
    uint32_t lines = 0;
    uint32_t pos = append.append(text.data(), (uint32_t)text.size(), &lines);
    constants[i] = {pos, lines};
  }

  for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
    const RegexMatch &match = *it;

    Shard *replacement = nullptr;
    for (size_t i = 0; i < replace_parts.size(); ++i) {
      const ReplacePart &part = replace_parts[i];
      switch (part.type) {
      case ReplacePart::PartType::Constant: {
        const std::string &text = std::get<std::string>(part.value);
        const ConstantRef &ref = constants[i];
        Shard *piece = new Petal((uint32_t)text.size(), ref.lines, &append, ref.pos);
        append_piece(replacement, piece);
        break;
      }
      case ReplacePart::PartType::FullMatch:
        append_piece(replacement, extract_range(root, match.start, match.end));
        break;
      case ReplacePart::PartType::CaptureGroup: {
        uint8_t idx = std::get<uint8_t>(part.value);
        if (idx <= match.groups.size()) {
          const RegexGroup &group = match.groups[idx - 1];
          if (group.matched)
            append_piece(replacement, extract_range(root, group.start, group.end));
        }
        break;
      }
      }
    }

    auto [left, rest] = split_shard(root, match.start);
    auto [dropped, right] = split_shard(rest, match.end - match.start);
    Shard::release(dropped);
    Shard::release(rest);

    Shard *new_root;
    if (replacement) {
      Shard *left2 = concat_shard(left, replacement);
      Shard::release(left);
      Shard::release(replacement);
      new_root = concat_shard(left2, right);
      Shard::release(left2);
    } else {
      new_root = concat_shard(left, right);
      Shard::release(left);
    }
    Shard::release(right);
    Shard::release(root);
    root = new_root;
  }
}
