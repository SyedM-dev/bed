#pragma once

#include "internal/vase/vase.h"
#include "pch.h"

namespace crib::commands::ed {
std::string summary();
void help();
void run(std::vector<std::string>);
} // namespace crib::commands::ed
