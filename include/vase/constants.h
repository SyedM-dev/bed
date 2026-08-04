#pragma once

#include "pch.h"

constexpr uint64_t APPEND_CHUNK_SIZE = 64 * 1024;
constexpr uint64_t PETAL_SIZE_MAX = 32 * 1024;

static_assert(
  APPEND_CHUNK_SIZE > PETAL_SIZE_MAX,
  "PETAL_SIZE_MAX must be smaller than APPEND_CHUNK_SIZE"
);
