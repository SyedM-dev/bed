#include "io/file.h"
#include "pch.h"
#include "vase/shard.h"
#include "vase/vase.h"

int main(int argc, char *argv[]) {
  /*const char *text_o =
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n"
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";

  uint32_t len = strlen(text_o);
  char *text = strdup(text_o);*/

  const char *path;

  if (argc >= 2)
    path = argv[1];
  else
    throw "Please give filename.";

  uint32_t len;
  char *text;
  read_file(path, &text, &len);

  auto start = std::chrono::steady_clock::now();
  Vase vase = Vase(text, len);
  auto end = std::chrono::steady_clock::now();
  std::cout << "Time to load: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";

  std::cout << "Press Enter to continue...";
  std::cin.get();

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
  // print_regex(matches); // debug
  start = std::chrono::steady_clock::now();
  std::vector<RegexMatch> matches = regex_search(vase.root, "here", 0, vase.length(), "");
  end = std::chrono::steady_clock::now();
  std::cout << "Time search: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";

  // print_shard(vase.root); // debug

  /*LineIterator it(vase.root, 0);
  std::string line;
  while (it.next(line))
    std::cout << line << "\n";*/

  /*start = std::chrono::steady_clock::now();
  vase.regex_search_replace("here", 0, vase.length(), "hello", "");
  end = std::chrono::steady_clock::now();
  std::cout << "Time replace: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";*/

  std::cout << "\n\n"
            << (int)vase.root->height << " append:" << (int)vase.append.current_offset;

  std::cout << "Press Enter to continue...";
  std::cin.get();

  /*ChunkIterator it2(vase.root);
  const char *data;
  uint32_t length;
  while (it2.next(&data, &length))
    std::cout << std::string(data, length);*/

  // std::cout          << vase.to_string();

  return 0;
}
