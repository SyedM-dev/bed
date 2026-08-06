#include "pch.h"
#include "vase/iterators/line.h"
#include "vase/vase.h"

int main(int argc, char *argv[]) {
  if (argc < 2)
    throw std::runtime_error("Please give filename.");

  Vase vase = Vase(argv[1], "/tmp");

  Point p = {UINT64_MAX, UINT64_MAX};
  vase.insert(&p, 'c');

  printf("%s\n\n", vase.to_string().c_str());

  printf("%lu bytes\n"
         "%lu lines\n"
         "\n",
         vase.length(),
         vase.root->lines);

  return 0;
}
