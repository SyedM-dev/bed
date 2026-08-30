#pragma once

#include "constants.h"
#include "definitions.h"
#include "iterators/line.h"
#include "pch.h"
#include "shard.h"
#include "storage/append.h"
#include "storage/original.h"

namespace bed::internal::vase {
struct Point {
  uint64_t row;
  uint64_t col;
};

struct Range {
  Point start;
  Point end;
};

struct RegexGroup {
  uint64_t start{UINT64_MAX};
  uint64_t end{UINT64_MAX};
};

struct RegexMatch {
  uint64_t start;
  uint64_t end;
  RegexGroup groups[9]{};
};

struct ReplacePart {
  enum struct PartType {
    FullMatch,
    CaptureGroup,
    Constant
  } type;
  std::variant<uint8_t, Shard *> value;
};

uint64_t offset_of(Shard *root, uint64_t line);
uint64_t offset_of(Shard *root, Point point);
Point point_of(Shard *root, uint64_t offset);

std::string to_string(Shard *root);
std::string to_string(Shard *root, Range range);

Shard *insert(AppendStorage *ap, Shard *root, Shard *text, uint64_t line);
Shard *erase(Shard *root, uint64_t start, uint64_t end);
Shard *join(Shard *root, uint64_t start, uint64_t end);
Shard *copy(Shard *root, uint64_t start, uint64_t end);

uint64_t find_next(Shard *root, std::string_view pattern, uint64_t start);
uint64_t find_prev(Shard *root, std::string_view pattern, uint64_t start);

Shard *substitute(
  AppendStorage *ap, Shard *root,
  std::string_view pattern, uint64_t start, uint64_t end,
  std::string_view replace, std::string_view options,
  const std::function<void(
    uint64_t line, uint64_t old_lines, uint64_t new_lines
  )> &on_edit = nullptr
);

// internal

void _insert(AppendStorage *ap, Shard **root, Point *point, const char *data, uint64_t len);
Shard *insert(AppendStorage *ap, Shard *root, Point *point, char key);
Shard *insert(AppendStorage *ap, Shard *root, Point *point, const char *data, uint64_t len);

Shard *erase(Shard *root, Range range);
Shard *replace(AppendStorage *ap, Shard *root, Range range, const char *data, uint64_t len);

std::vector<ReplacePart> parse_replace(AppendStorage *ap, std::string_view s);
std::vector<RegexMatch> _regex_search(
  Shard *root, std::string_view pattern, uint64_t start_offset, uint64_t end_offset, std::string_view options
);
} // namespace bed::internal::vase
