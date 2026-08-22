#pragma once

#include "pch.h"

namespace bed::internal::marks {
struct MarksEngine {
  uint64_t marks[1 << UINT8_WIDTH]{UINT64_MAX};

  MarksEngine() {
    for (auto &m : marks)
      m = UINT64_MAX;
  }

  uint64_t get(uint8_t m) {
    return marks[m];
  }

  void set(uint8_t m, uint64_t line) {
    marks[m] = line;
  }

  void insert(uint64_t start, uint64_t count) {
    for (uint16_t i = 0; i <= UINT8_MAX; ++i) {
      if (marks[i] == UINT64_MAX)
        continue;
      if (marks[i] >= start)
        marks[i] += count;
    }
  }

  void erase(uint64_t start, uint64_t count) {
    for (uint16_t i = 0; i <= UINT8_MAX; ++i) {
      if (marks[i] == UINT64_MAX)
        continue;
      if (marks[i] >= start) {
        if (marks[i] < start + count)
          marks[i] = UINT64_MAX;
        else
          marks[i] -= count;
      }
    }
  }

  void collapse(uint64_t start, uint64_t count) {
    for (uint16_t i = 0; i <= UINT8_MAX; ++i) {
      if (marks[i] == UINT64_MAX)
        continue;
      if (marks[i] >= start) {
        if (marks[i] < start + count)
          marks[i] = start;
        else
          marks[i] -= count;
      }
    }
  }
};
} // namespace bed::internal::marks
