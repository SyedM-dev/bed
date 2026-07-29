#include "vase/iterators/line.h"

LineIterator::LineIterator(Shard *r, uint32_t start_line)
    : offset(offset_of(r, start_line, 0)), it(r, offset) {}

LineIterator::~LineIterator() {};

bool LineIterator::next(std::string &line) {
  line.clear();
  while (true) {
    if (remaining == 0) {
      uint32_t got;
      if (!it.next(&cursor, &got)) {
        eof = true;
        break;
      }
      remaining = got;
    }
    const char *nl = (const char *)memchr(cursor, '\n', remaining);
    if (nl) {
      const char *end = nl;
      size_t len = end - cursor;
      line.append(cursor, len);
      cursor = end + 1;
      remaining -= (uint32_t)(len + 1);
      return true;
    }
    line.append(cursor, remaining);
    cursor += remaining;
    remaining = 0;
  }
  return !line.empty();
}
