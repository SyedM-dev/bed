#include "pch.h"
#include "vase/iterators/chunk.h"
#include "vase/iterators/line.h"
#include "vase/vase.h"

int main(int argc, char *argv[]) {
  if (argc < 2)
    throw std::runtime_error("Please give filename.");

  Vase vase = Vase(std::string(argv[1]));

  std::cout << "\n"
            << vase.to_string()
            << "\n";

  return 0;
}
