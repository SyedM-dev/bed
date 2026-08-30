#include "internal/vase/vase.h"

namespace bed::internal::vase {
std::string to_string(Shard *root) {
  std::string out;
  if (!root)
    return out;
  PetalIterator it(root, Direction::Forward);
  it.seek_offset(0);
  const char *data;
  uint64_t len;
  while (it.next(&data, &len))
    out.append(data, len);
  return out;
}

std::string to_string(Shard *root, Range range) {
  std::string out;
  if (!root)
    return out;
  PetalIterator it(root, Direction::Forward);
  uint64_t start = offset_of(root, range.start);
  it.seek_offset(start);
  const char *data;
  uint64_t len;
  uint64_t remaining = offset_of(root, range.end) - start;
  while (remaining && it.next(&data, &len)) {
    uint64_t n = std::min(len, remaining);
    out.append(data, n);
    remaining -= n;
  }
  return out;
}

Shard *insert(AppendStorage *ap, Shard *root, Point *point, char key) {
  uint64_t pos = ap->append(key);
  Shard *inserted = new Petal(1, key == '\n', ap, pos);
  auto [left, right] = Shard::split(root, offset_of(root, *point));
  Shard *left2 = Shard::append(left, inserted);
  Shard::release(left);
  Shard::release(inserted);
  Shard *new_root = Shard::concat(left2, right);
  Shard::release(left2);
  Shard::release(right);
  Shard::release(root);
  if (key == '\n')
    *point = {point->row + 1, 0};
  else
    point->col++;
  return new_root;
}

Shard *insert(AppendStorage *ap, Shard *root, Point *point, const char *data, uint64_t len) {
  while (len) {
    uint64_t chunk_size = std::min<uint64_t>(len, PETAL_SIZE_MAX);
    _insert(ap, &root, point, data, chunk_size);
    len -= chunk_size;
    data += chunk_size;
  }
  return root;
}

void _insert(AppendStorage *ap, Shard **root, Point *point, const char *data, uint64_t len) {
  if (len == 0)
    return;
  uint64_t offset = offset_of(*root, *point);
  uint64_t pos = ap->append(data, len);
  uint64_t lines = 0;
  const char *start = data;
  const char *last_line = start;
  const char *end = start + len;
  while ((data = (const char *)memchr(data, '\n', end - data))) {
    ++lines;
    last_line = ++data;
  }
  uint64_t col = 0;
  uint64_t remaining = end - last_line;
  while (remaining) {
    uint64_t n = grapheme_next_character_break_utf8(last_line, remaining);
    last_line += n;
    remaining -= n;
    ++col;
  }
  if (lines) {
    point->row += lines;
    point->col = col;
  } else {
    point->col += col;
  }
  Shard *inserted = new Petal(len, lines, ap, pos);
  auto [left, right] = Shard::split(*root, offset);
  Shard *left2 = Shard::append(left, inserted);
  Shard::release(left);
  Shard::release(inserted);
  Shard *new_root = Shard::concat(left2, right);
  Shard::release(left2);
  Shard::release(right);
  Shard::release(*root);
  *root = new_root;
}

Shard *erase(Shard *root, Range range) {
  Point start = range.start;
  Point end = range.end;
  uint64_t start_offset = offset_of(root, start);
  uint64_t end_offset = offset_of(root, end);
  uint64_t count = end_offset - start_offset;
  auto [a, b] = Shard::split(root, start_offset);
  auto [d, c] = Shard::split(b, count);
  Shard *new_root = Shard::concat(a, c);
  Shard::release(a);
  Shard::release(b);
  Shard::release(c);
  Shard::release(d);
  Shard::release(root);
  return new_root;
}

Shard *replace(AppendStorage *ap, Shard *root, Range range, const char *data, uint64_t len) {
  root = erase(root, range);
  return insert(ap, root, &range.start, data, len);
}

uint64_t offset_of(Shard *root, Point point) {
  return offset_of(root, point.row) + point.col;
}

uint64_t offset_of(Shard *root, uint64_t line) {
  if (!root)
    return 0;
  if (line > root->lines)
    throw ed_error("line out of range");
  uint64_t offset = 0;
  Shard *curr = root;
  while (curr->kind == Shard::Kind::Branch) {
    auto *b = (Branch *)curr;
    if (line <= b->left->lines) {
      curr = b->left;
    } else {
      line -= b->left->lines;
      offset += b->left->length;
      curr = b->right;
    }
  }
  auto *petal = (Petal *)curr;
  if (line == 0)
    return offset;
  const char *text = petal->source->read(petal->pos);
  uint64_t local = 0;
  while (line--) {
    const char *nl = (const char *)memchr(text + local, '\n', petal->length - local);
    if (!nl)
      throw std::runtime_error("leaf line count is wrong.");
    local = (nl - text) + 1;
  }
  return offset + local;
}

static Shard *newline(AppendStorage *ap) {
  uint64_t pos = ap->append('\n');
  return new Petal(1, true, ap, pos);
}

Shard *insert(AppendStorage *ap, Shard *root, Shard *text, uint64_t line) {
  if (!root) {
    if (line != 0)
      throw ed_error("line out of range");
    Shard::retain(text);
    return text;
  }
  if (line > root->lines + 1)
    throw ed_error("line out of range");
  Shard::retain(text);
  Shard *nl = newline(ap);
  if (line <= root->lines) {
    uint64_t offset = offset_of(root, line);
    auto [left, right] = Shard::split(root, offset);
    Shard *middle = Shard::concat(text, nl);
    Shard::release(text);
    Shard::release(nl);
    Shard *new_root = Shard::concat(left, middle);
    Shard::release(left);
    Shard::release(middle);
    middle = Shard::concat(new_root, right);
    Shard::release(new_root);
    Shard::release(right);
    Shard::release(root);
    return middle;
  }
  Shard *new_root = Shard::concat(root, nl);
  Shard::release(root);
  Shard::release(nl);
  Shard *result = Shard::concat(new_root, text);
  Shard::release(new_root);
  Shard::release(text);
  return result;
}

Shard *erase(Shard *root, uint64_t start, uint64_t end) {
  if (!start || !end)
    throw ed_error("Invalid range.");
  start--;
  end--;
  if (!root)
    throw ed_error("line range out of bounds");
  uint64_t line_count = root->lines + 1;
  if (start > end || end >= line_count)
    throw ed_error("line range out of bounds");
  uint64_t start_offset = offset_of(root, start);
  uint64_t end_offset =
    (end + 1 == line_count)
      ? root->length
      : offset_of(root, end + 1);
  auto [left, rest] = Shard::split(root, start_offset);
  auto [middle, right] = Shard::split(rest, end_offset - start_offset);
  Shard *new_root = Shard::concat(left, right);
  Shard::release(left);
  Shard::release(rest);
  Shard::release(middle);
  Shard::release(right);
  Shard::release(root);
  return new_root;
}

Shard *join(Shard *root, uint64_t start, uint64_t end) {
  if (!start || !end)
    throw ed_error("Invalid range.");
  start--;
  end--;
  if (!root)
    throw ed_error("line range out of bounds");
  uint64_t line_count = root->lines + 1;
  if (start >= end || end >= line_count)
    throw ed_error("line range out of bounds");

  uint64_t offset = offset_of(root, start);
  std::vector<Shard *> pieces;
  pieces.reserve(end - start + 3);
  auto [a, b] = Shard::split(root, offset);
  pieces.push_back(a);
  for (uint64_t line = start; line < end; line++) {
    uint64_t nl_pos = offset_of(b, 1) - 1;
    auto [content, rest] = Shard::split(b, nl_pos);
    Shard::release(b);
    auto [nl, next_b] = Shard::split(rest, 1);
    Shard::release(rest);
    Shard::release(nl);
    pieces.push_back(content);
    b = next_b;
  }
  pieces.push_back(b);
  Shard *joined = Shard::build(pieces.data(), 0, pieces.size());
  Shard::release(root);
  return joined;
}

Shard *copy(Shard *root, uint64_t start, uint64_t end) {
  if (!start || !end)
    throw ed_error("Invalid range.");
  start--;
  end--;
  if (!root)
    throw ed_error("line range out of bounds");
  uint64_t line_count = root->lines + 1;
  if (start > end || end >= line_count)
    throw ed_error("line range out of bounds");
  uint64_t start_offset = offset_of(root, start);
  uint64_t end_offset =
    (end + 1 == line_count)
      ? root->length
      : offset_of(root, end + 1);
  auto [left, rest] = Shard::split(root, start_offset);
  auto [middle, right] = Shard::split(rest, end_offset - start_offset);
  Shard::release(left);
  Shard::release(rest);
  Shard::release(right);
  Shard::release(root);
  return middle;
}
} // namespace bed::internal::vase
