#include "io/file.h"
#include "pch.h"
#include "vase/shard.h"
#include "vase/vase.h"

int main() {
  const char *text_o =
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";

  uint32_t len = strlen(text_o);
  char *text = strdup(text_o);

  Vase vase = Vase(text, len);

  vase.insert(1, "bcgr\ntt", 5);
  vase.insert(2, "cgr\ntt", 5);
  vase.insert(58, "gr\ntt", 5);
  vase.insert(100, "gr\ntt", 5);
  vase.insert(20, "gr\ntt", 5);

  vase.type(5, '1');
  vase.type(6, '2');
  vase.type(7, '3');
  vase.type(8, '4');

  vase.erase(offset_of(vase.root, 7, 3), -3);

  // supports gisxUn and m as inverse (as multiline is default.)
  std::vector<RegexMatch> matches = regex_search(vase.root, "a(.)c", 0, vase.length(), "s");
  print_regex(matches); // debug

  print_shard(vase.root); // debug

  LineIterator it(vase.root, 0);
  std::string line;
  while (it.next(line))
    std::cout << line << "\n";

  vase.regex_search_replace("a(.)c", 0, vase.length(), "|$1|", "g");

  std::cout << "\n\n";

  ChunkIterator it2(vase.root);
  const char *data;
  uint32_t length;
  while (it2.next(&data, &length))
    std::cout << std::string(data, length);

  std::cout << "\n\n"
            << vase.to_string();

  return 0;
}
