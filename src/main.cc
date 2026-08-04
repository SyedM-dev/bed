#include "pch.h"
#include "vase/iterators/chunk.h"
#include "vase/iterators/line.h"
#include "vase/vase.h"

int main(int argc, char *argv[]) {
  if (argc < 2)
    throw std::runtime_error("Please give filename.");

  Vase vase = Vase(std::string(argv[1]));

  Point p = {500, 1000};
  vase.insert(&p, 'x');

  printf("\n"
         "%lu bytes\n"
         "%lu lines\n"
         "\n",
         vase.length(),
         vase.root->lines);

  return 0;
}
