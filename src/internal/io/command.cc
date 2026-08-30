#include "internal/ui/command.h"
#include "bed.h"
#include "internal/io/io.h"

namespace bed::internal::io {
std::pair<std::string, bool> IO::get_command(BEd &ctx) {
  ui::CommandIO cio(ctx, *this);
  return cio.run();
}
} // namespace bed::internal::io
