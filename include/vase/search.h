#pragma once

#include "pch.h"
#include "vase/shard.h"

struct RegexGroup {
  uint32_t start;
  uint32_t end;
  bool matched;
};

struct RegexMatch {
  uint32_t start;
  uint32_t end;
  std::vector<RegexGroup> groups{};
};

const std::vector<RegexMatch> regex_search(Shard *root, std::string_view pattern_str, uint32_t start_offset, uint32_t end_offset, uint32_t flags, bool global);
void print_regex(const std::vector<RegexMatch> &matches);
