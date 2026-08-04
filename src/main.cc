#include "pch.h"
#include "vase/iterators/chunk.h"
#include "vase/iterators/line.h"
#include "vase/vase.h"

int main(int argc, char *argv[]) {
  if (argc < 2)
    throw std::runtime_error("Please give filename.");

  Vase vase = Vase(std::string(argv[1]));

  auto matches = vase.regex_search("hello", {{0, 0}, {vase.root->lines, UINT64_MAX}}, "g");

  std::cout << "\n"
            << (int)vase.length() << " bytes\n"
            << (int)vase.root->lines << " lines\n"
            << (int)matches.size() << " matches\n"
            << "\n";

  return 0;
}
