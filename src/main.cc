#include "bed.h"
#include "pch.h"

int main(int argc, char *argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  try {
    bed::internal::io::IO io = bed::internal::io::IO();
    bed::BEd ed(args, io);
    ed.run();
  } catch (bed::fatal_error &e) {
    if (e.code)
      printf("Fatal error: %s\n", e.what());
    return e.code;
  }
#ifndef DEBUG
  catch (...) {
    printf("Unexpected error.\n");
    return 1;
  }
#endif
  return 0;
}
