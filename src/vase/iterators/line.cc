#include "vase/iterators/line.h"

LineIterator::LineIterator(Shard *r, uint64_t start_line, Direction dir)
    : it(r, dir), dir(dir) {
  int b = start_line == UINT64_MAX ? 0 : dir == Direction::Backward;
  it.seek_line(start_line + b);
  if (!it.next(&chunk, &len)) {
    chunk = nullptr;
    return;
  }
  if (chunk == nullptr)
    at_end = true;
  if (dir == Direction::Forward)
    chunk_offset = it.byte_offset() - len;
  else
    chunk_offset = it.byte_offset();
}

uint64_t LineIterator::byte_offset() {
  return offset;
}

bool LineIterator::next(std::string &line) {
  line.clear();
  if (at_end) {
    offset = chunk_offset;
    at_end = false;
    return true;
  }
  if (!chunk)
    return false;
  if (dir == Direction::Forward) {
    offset = chunk_offset;
    while (true) {
      const char *nl = (const char *)memchr(chunk, '\n', len);
      if (!nl) {
        if (len && chunk[len - 1] == '\r')
          --len;
        line.append(chunk, len);
        chunk_offset += len;
        if (!it.next(&chunk, &len)) {
          chunk = nullptr;
          return true;
        }
        if (chunk == nullptr)
          return true;
        chunk_offset = it.byte_offset() - len;
        continue;
      }
      const char *end = nl;
      if (end > chunk && *(end - 1) == '\r')
        --end;
      line.append(chunk, end);
      uint64_t consumed = (nl - chunk) + 1;
      chunk += consumed;
      len -= consumed;
      chunk_offset += consumed;
      return true;
    }
  } else {
    while (true) {
#if defined(__GLIBC__) || defined(__APPLE__)
      const char *nl = (const char *)memrchr(chunk, '\n', len);
#else
      const char *nl = nullptr;
      uint64_t i = len;
      while (i--) {
        if (chunk[i] == '\n') {
          nl = chunk + i;
          break;
        }
      }
#endif
      if (!nl) {
        if (len && chunk[len - 1] == '\r')
          --len;
        line.insert(0, chunk, len);
        chunk_offset -= len;
        if (!it.next(&chunk, &len)) {
          chunk = nullptr;
          return true;
        }
        chunk_offset = it.byte_offset();
        continue;
      }
      const char *end = chunk + len;
      const char *start = nl + 1;
      const char *line_end = end;
      if (line_end > start && *(line_end - 1) == '\r')
        --line_end;
      line.insert(0, start, line_end - start);
      len = nl - chunk;
      offset = chunk_offset + (start - chunk);
      return true;
    }
  }
}
