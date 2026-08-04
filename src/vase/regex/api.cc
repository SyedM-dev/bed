#include "vase/vase.h"

std::vector<ReplacePart> Vase::parse_replace(std::string_view s) {
  std::vector<ReplacePart> parts;
  std::string constant;
  auto flush_constant = [&]() {
    if (!constant.empty()) {
      uint64_t lines = 0;
      uint64_t pos = append->append(constant.data(), (uint64_t)constant.size());
      parts.push_back(
        ReplacePart{
          .type = ReplacePart::PartType::Constant,
          .value = new Petal((uint64_t)constant.size(), lines, append, pos)
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
  std::string_view pattern, Range range,
  std::string_view replace, std::string_view options
) {
  std::vector<RegexMatch> matches = _regex_search(pattern, range, options);
  if (matches.empty())
    return;

  std::vector<ReplacePart> replace_parts = parse_replace(replace);

  std::vector<Shard *> pieces;
  pieces.reserve(matches.size() * 2 + 1);

  Shard *remaining = root;
  Shard::retain(remaining);
  uint64_t cursor = 0;

  for (const RegexMatch &match : matches) {
    uint64_t gap = match.start - cursor;
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
            uint64_t ls = group.start - match.start;
            uint64_t le = group.end - match.start;
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

std::vector<Range> Vase::regex_search(
  std::string_view pattern, Range range, std::string_view options
) {
  std::vector<RegexMatch> matches = _regex_search(pattern, range, options);
  if (matches.empty())
    return {};
  std::vector<Range> result;
  result.reserve(matches.size());
  for (auto match : matches)
    result.push_back({point_of(match.start), point_of(match.end)});
  return result;
}
