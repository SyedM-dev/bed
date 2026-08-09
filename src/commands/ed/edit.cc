#include "commands/ed/ed.h"

namespace crib::commands::ed {
void Ed::append(std::string text, uint64_t line) {
  using namespace crib::internal::vase;
  Point p = {line, 0};
  if (!vase.lines()) {
    text.pop_back();
  } else if (line == vase.lines()) {
    p.row--;
    p.col = UINT64_MAX;
    text = "\n" + text;
    text.pop_back();
  }
  vase.insert(&p, text);
}

void Ed::remove(uint64_t start_line, uint64_t end_line) {
  vase.erase({{start_line - 1, 0}, {end_line, 0}});
}
} // namespace crib::commands::ed
