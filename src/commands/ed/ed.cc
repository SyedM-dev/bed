#include "commands/ed/ed.h"

namespace crib::commands::ed {
std::string summary() {
  return "A POSIX-compliant line editor.";
}
void help() {}
void run(int, char *[]) {
  return;
}
} // namespace crib::commands::ed
