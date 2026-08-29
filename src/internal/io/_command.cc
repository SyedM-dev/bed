#include "bed.h"
#include "internal/io/command.h"
#include "internal/io/io.h"

namespace bed::internal::io {
std::pair<std::string, bool> IO::get_command(BEd &ctx) {
  CommandIO cio(ctx, *this);
  return cio.run();
}
} // namespace bed::internal::io
