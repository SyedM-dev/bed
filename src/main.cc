#include "io/file.h"
#include "pch.h"
#include "vase/shard.h"
#include "vase/vase.h"

int main() {
  const char *text_o =
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqr\n"
    "st\n"
    "uv\n"
    "wx\n"
    "yzabcdefghijklmnopqrstuvwxyz\n";

  uint32_t len = strlen(text_o);
  char *text = strdup(text_o);

  Vase vase = Vase(text, len);

  vase.insert(14, "gr\ntt", 5);
  vase.insert(58, "gr\ntt", 5);
  vase.insert(100, "gr\ntt", 5);
  vase.insert(20, "gr\ntt", 5);

  vase.type(5, '1');
  vase.type(6, '2');
  vase.type(7, '3');
  vase.type(8, '4');

  vase.erase(vase.offset_of(7, 3), -3);

  print_shard(vase.root);

  std::cout << (int)vase.offset_of(6, 0) << "\n\n";

  LineIterator it(vase.root, 3);
  std::string line;
  while (it.next(line))
    std::cout << line << "\n";

  std::cout << "\n\n"
            << vase.to_string();

  return 0;
}
