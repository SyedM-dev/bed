#include "vase/iterators/line.h"

LineIterator::LineIterator(Shard *r, uint32_t start_line) : it(r) {
  it.seek_line(start_line);
}

bool LineIterator::next(std::string &line) {
  line.clear();
  line_offset = it.byte_offset() - remaining;
  while (true) {
    if (remaining == 0) {
      uint32_t got;
      if (!it.next(&cursor, &got)) {
        eof = true;
        break;
      }
      line_offset = it.byte_offset() - remaining;
      remaining = got;
    }
    const char *nl = (const char *)memchr(cursor, '\n', remaining);
    if (nl) {
      const char *end = nl;
      uint32_t len = nl - cursor;
      if (len && cursor[len - 1] == '\r')
        --len;
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

uint32_t LineIterator::byte_offset() {
  return line_offset;
}
