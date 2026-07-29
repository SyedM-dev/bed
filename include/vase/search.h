#pragma once

#include "pch.h"
#include "vase/shard.h"

struct RegexGroup {
  uint32_t start{UINT32_MAX};
  uint32_t end{UINT32_MAX};
};

struct RegexMatch {
  uint32_t start;
  uint32_t end;
  RegexGroup groups[9]{};
};

std::vector<RegexMatch> regex_search(
  Shard *root, std::string_view pattern_str,
  uint32_t start_offset, uint32_t end_offset,
  std::string_view options
);
