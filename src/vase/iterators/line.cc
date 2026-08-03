#include "vase/iterators/line.h"

LineIterator::LineIterator(Shard *r, uint32_t start_line, Direction dir)
    : it(r, dir), dir(dir) {
  it.seek_line(start_line + (dir == Direction::Backward));
  if (!it.next(&chunk, &len))
    chunk = nullptr;
}

uint32_t LineIterator::byte_offset() {
  return 0;
}

bool LineIterator::next(std::string &line) {
  line.clear();
  if (!chunk)
    return false;
  if (dir == Direction::Forward) {
    while (true) {
      const char *nl = (const char *)memchr(chunk, '\n', len);
      if (!nl) {
        if (len && chunk[len - 1] == '\r')
          --len;
        line.append(chunk, len);
        if (!it.next(&chunk, &len)) {
          chunk = nullptr;
          return true;
        }
        continue;
      }
      const char *end = nl;
      if (end > chunk && *(end - 1) == '\r')
        --end;
      line.append(chunk, end);
      uint32_t consumed = (nl - chunk) + 1;
      chunk += consumed;
      len -= consumed;
      return true;
    }
  } else {
    while (true) {
      const char *nl = (const char *)memrchr(chunk, '\n', len);
      if (!nl) {
        if (len && chunk[len - 1] == '\n')
          --len;
        if (len && chunk[len - 1] == '\r')
          --len;
        line.insert(0, chunk, len);
        if (!it.next(&chunk, &len)) {
          chunk = nullptr;
          return true;
        }
        continue;
      }
      const char *end = chunk + len;
      const char *start = nl + 1;
      const char *line_end = end;
      if (line_end > start && *(line_end - 1) == '\r')
        --line_end;
      line.insert(0, start, line_end - start);
      len = nl - chunk;
      return true;
    }
  }
}
