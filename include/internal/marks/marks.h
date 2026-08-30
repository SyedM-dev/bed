#pragma once

#include "internal/buffer/buffer.h"
#include "pch.h"

namespace bed::internal::marks {
struct MarksEngine {
  buffer::Line marks[256];

  MarksEngine() {
    for (auto &m : marks)
      m = {"", UINT64_MAX};
  }

  buffer::Line &get(uint8_t m) {
    return marks[m];
  }

  void insert(std::string bufname, uint64_t start, uint64_t count) {
    for (uint16_t i = 0; i <= UINT8_MAX; ++i) {
      if (marks[i].buffername != bufname)
        continue;
      if (marks[i].number == UINT64_MAX)
        continue;
      if (marks[i].number >= start)
        marks[i].number += count;
    }
  }

  void erase(std::string bufname, uint64_t start, uint64_t count) {
    for (uint16_t i = 0; i <= UINT8_MAX; ++i) {
      if (marks[i].buffername != bufname)
        continue;
      if (marks[i].number == UINT64_MAX)
        continue;
      if (marks[i].number >= start) {
        if (marks[i].number < start + count)
          marks[i].number = UINT64_MAX;
        else
          marks[i].number -= count;
      }
    }
  }

  void collapse(std::string bufname, uint64_t start, uint64_t count) {
    for (uint16_t i = 0; i <= UINT8_MAX; ++i) {
      if (marks[i].buffername != bufname)
        continue;
      if (marks[i].number == UINT64_MAX)
        continue;
      if (marks[i].number > start) {
        if (marks[i].number <= start + count)
          marks[i].number = start;
        else
          marks[i].number -= count;
      }
    }
  }
};
} // namespace bed::internal::marks
