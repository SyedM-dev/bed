#include "vase/iterators/line.h"

LineIterator::LineIterator(Shard *root, uint64_t line_num, Direction dir)
    : it(root, dir), dir(dir) {
  it.seek_line(line_num);
  it.next(&chunk, &len);
  current_offset = it.byte_offset();
  last_line_offset = current_offset;
}

bool LineIterator::next(std::string *line) {
  if (!line || !chunk)
    return false;
  last_line_offset = current_offset;
  line->clear();
  if (dir == Direction::Forward) {
  retry_forward:
    const char *nl = (const char *)memchr(chunk, '\n', len);
    if (!nl) {
      if (len && chunk[len - 1] == '\r')
        len--;
      line->append(chunk, len);
      current_offset += len;
      if (!it.next(&chunk, &len))
        return true;
      goto retry_forward;
    }
    const char *end = nl;
    if (end - chunk > 0 && *(end - 1) == '\r')
      end--;
    line->append(chunk, end);
    size_t consumed = (nl - chunk) + 1;
    chunk += consumed;
    len -= consumed;
    current_offset += consumed;
    return true;
  } else {
  retry_backward:
#if defined(__GLIBC__) || defined(__APPLE__) || defined(__FreeBSD__)
    const char *nl = (const char *)memrchr(chunk, '\n', len);
#else
    const char *nl = nullptr;
    const char *p = chunk + len;
    while (!nl && p != chunk)
      if (*(--p) == '\n')
        nl = p;
#endif
    if (!nl) {
      if (len && chunk[len - 1] == '\r')
        len--;
      line->insert(0, chunk, len);
      current_offset -= len;
      if (!it.next(&chunk, &len))
        return true;
      goto retry_backward;
    }
    const char *start = nl + 1;
    const char *line_end = chunk + len;
    const size_t consumed = line_end - nl;
    const char *end = line_end;
    if (end > start && *(end - 1) == '\r')
      end--;
    if (end > start)
      line->insert(0, start, end - start);
    len = nl - chunk;
    current_offset -= consumed;
    return true;
  }
}

uint64_t LineIterator::byte_offset() {
  return last_line_offset;
}
