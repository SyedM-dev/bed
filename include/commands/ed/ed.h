#pragma once

#include "internal/vase/vase.h"
#include "pch.h"

namespace crib::commands::ed {
std::string summary();
void help();
void run(int argc, char *argv[]);
} // namespace crib::commands::ed
