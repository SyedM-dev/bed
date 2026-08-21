#include "bed.h"
#include "pch.h"

int main(int argc, char *argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  try {
    bed::BEd ed(args);
    ed.run();
  } catch (bed::fatal_error &e) {
    if (e.code)
      std::cout << "Fatal error: " << e.what() << std::endl;
    return e.code;
  }
#ifndef DEBUG
  catch (...) {
    std::cout << "Unexpected error." << std::endl;
    return 1;
  }
#endif
  return 0;
}
