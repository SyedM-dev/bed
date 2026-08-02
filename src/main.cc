#include "io/file.h"
#include "pch.h"
#include "vase/iterators/chunk.h"
#include "vase/iterators/line.h"
#include "vase/shard.h"
#include "vase/vase.h"

int main(int argc, char *argv[]) {

#ifdef DEBUG

  const char *text_o =
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz1\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz2\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz3\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz4";

  uint32_t len = strlen(text_o);
  char *text = strdup(text_o);

#else

  const char *path;

  if (argc >= 2)
    path = argv[1];
  else
    throw std::runtime_error("Please give filename.");

  uint32_t len;
  char *text;
  read_file(path, &text, &len);

#endif

  Vase vase = Vase(text, len);

  return 0;
}
