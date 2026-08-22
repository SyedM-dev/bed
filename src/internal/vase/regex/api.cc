#include "internal/vase/vase.h"

namespace bed::internal::vase {
std::vector<Vase::ReplacePart> Vase::parse_replace(std::string_view s) {
  std::vector<ReplacePart> parts;
  std::string constant;
  auto flush_constant = [&]() {
    if (!constant.empty()) {
      uint64_t lines = 0;
      const char *p = constant.data();
      const char *end = p + constant.size();
      while ((p = (const char *)memchr(p, '\n', end - p))) {
        ++lines;
        ++p;
      }
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
      auto [keep, rest] = Shard::split(remaining, gap);
      Shard::release(remaining);
      pieces.push_back(keep);
      remaining = rest;
    }

    auto [dropped, rest2] = Shard::split(remaining, match.end - match.start);
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
          if (group.start > group.end) {
            uint64_t ls = group.start - match.start;
            uint64_t le = group.end - match.start;
            auto [a, b] = Shard::split(dropped, ls);
            auto [g, c] = Shard::split(b, le - ls);
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

  Shard *new_root = compact.empty() ? nullptr : Shard::build(compact.data(), 0, compact.size());
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

uint64_t Vase::find_next(std::string_view pattern, uint64_t start) {
  std::vector<RegexMatch> results;
  int errornumber;
  PCRE2_SIZE erroroffset;
  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)pattern.data(),
    pattern.size(),
    PCRE2_UTF | PCRE2_EXTENDED,
    &errornumber,
    &erroroffset,
    NULL
  );
  if (re == NULL)
    throw ed_error("Can't compile regex.");
  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, nullptr);
  if (!match_data) {
    pcre2_code_free(re);
    throw ed_error("Can't create regex match data.");
  }
  uint64_t at = (start + 1) % lines();
  LineIterator it(root, at, Direction::Forward);
  std::string line;
  while (it.next(&line)) {
    int rc = pcre2_match(re, (PCRE2_SPTR)line.data(), line.size(), 0, 0, match_data, nullptr);
    if (rc >= 0) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      return at;
    }
    if (rc != PCRE2_ERROR_NOMATCH) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      throw ed_error("Regex matching failed.");
    }
    at++;
  }
  at = 0;
  LineIterator it2(root, at, Direction::Forward);
  while (it2.next(&line)) {
    if (at >= start)
      break;
    int rc = pcre2_match(re, (PCRE2_SPTR)line.data(), line.size(), 0, 0, match_data, nullptr);
    if (rc >= 0) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      return at;
    }
    if (rc != PCRE2_ERROR_NOMATCH) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      throw ed_error("Regex matching failed.");
    }
    at++;
  }
  pcre2_match_data_free(match_data);
  pcre2_code_free(re);
  throw ed_error("No line matched.");
}

uint64_t Vase::find_prev(std::string_view pattern, uint64_t start) {
  std::vector<RegexMatch> results;
  int errornumber;
  PCRE2_SIZE erroroffset;
  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)pattern.data(),
    pattern.size(),
    PCRE2_UTF | PCRE2_EXTENDED,
    &errornumber,
    &erroroffset,
    NULL
  );
  if (re == NULL)
    throw ed_error("Can't compile regex.");
  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, nullptr);
  if (!match_data) {
    pcre2_code_free(re);
    throw ed_error("Can't create regex match data.");
  }
  uint64_t at = (start == 0 ? lines() : start) - 1;
  LineIterator it(root, at, Direction::Backward);
  std::string line;
  while (it.next(&line)) {
    int rc = pcre2_match(re, (PCRE2_SPTR)line.data(), line.size(), 0, 0, match_data, nullptr);
    if (rc >= 0) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      return at;
    }
    if (rc != PCRE2_ERROR_NOMATCH) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      throw ed_error("Regex matching failed.");
    }
    at--;
  }
  at = lines();
  LineIterator it2(root, at, Direction::Backward);
  while (it2.next(&line)) {
    if (at <= start)
      break;
    int rc = pcre2_match(re, (PCRE2_SPTR)line.data(), line.size(), 0, 0, match_data, nullptr);
    if (rc >= 0) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      return at;
    }
    if (rc != PCRE2_ERROR_NOMATCH) {
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      throw ed_error("Regex matching failed.");
    }
    at--;
  }
  pcre2_match_data_free(match_data);
  pcre2_code_free(re);
  throw ed_error("No line matched.");
}
} // namespace bed::internal::vase
