#pragma once

#include "pch.h"

namespace crib::cli {
extern bool use_colors;
void help();
int run(int argc, char *argv[]);
} // namespace crib::cli
