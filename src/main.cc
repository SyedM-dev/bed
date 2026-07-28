#include "io/file.h"
#include "pch.h"
#include "vase/shard.h"
#include "vase/vase.h"

int main() {
  char *text;
  uint32_t len;

  int s = read_file("./flake.nix", &text, &len);
  if (!s)
    return 1;

  Vase vase = Vase(text, len);

  print_shard(vase.root.ptr);

  return 0;
}
