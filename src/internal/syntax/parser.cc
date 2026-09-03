#include "internal/syntax/parser.h"

namespace bed::internal::syntax {
ParserSnapshot make_parser(vase::Shard *vase, uint64_t lines, Language *lang) {
  ParserSnapshot snap{nullptr, lang};
  if (lines == 0)
    return snap;
  if (lang)
    snap.root = ParseState::splice(*lang, nullptr, vase, 0, 0, lines);
  return snap;
}

ParserSnapshot retain(const ParserSnapshot &snap) {
  if (snap.root)
    ParseState::retain(snap.root);
  return snap;
}

void release(ParserSnapshot &snap) {
  if (snap.root && snap.lang)
    ParseState::release(*snap.lang, snap.root);
  snap.root = nullptr;
}

uint64_t next_closing(const ParserSnapshot &snap, uint64_t line) {
  if (!snap.root || !snap.lang)
    return line + 10;
  uint64_t relative = 0;
  TreeCursor c(*snap.lang, snap.root, line, &relative);
  uint64_t line_offset = line - relative;
  int level = 0;
  bool first_leaf = true;
  while (c.leaf) {
    auto *leaf = c.leaf;
    for (uint32_t i = 0; i < leaf->n; ++i) {
      uint16_t block = leaf->blocks[i];
      uint32_t pos = block & ParseStateLeaf::LINE_MASK;
      if (first_leaf && pos <= relative)
        continue;
      bool closing = block & ParseStateLeaf::IS_CLOSING;
      if (closing) {
        if (level == 0)
          return line_offset + pos;
        --level;
      } else {
        ++level;
      }
    }
    line_offset += leaf->lines();
    c.next();
    first_leaf = false;
  }
  return (snap.root->lines() - line > 10 ? line + 10 : snap.root->lines() - 1);
}

uint64_t prev_opening(const ParserSnapshot &snap, uint64_t line) {
  if (!snap.root || !snap.lang)
    return 0;
  uint64_t relative = 0;
  TreeCursor c(*snap.lang, snap.root, line, &relative);
  uint64_t line_offset = line - relative;
  int level = 0;
  bool first_leaf = true;
  while (c.leaf) {
    auto *leaf = c.leaf;
    for (int32_t i = (int32_t)leaf->n - 1; i >= 0; --i) {
      uint16_t block = leaf->blocks[i];
      uint32_t pos = block & ParseStateLeaf::LINE_MASK;
      if (first_leaf && pos >= relative)
        continue;
      bool closing = block & ParseStateLeaf::IS_CLOSING;
      if (!closing) {
        if (level == 0)
          return line_offset + pos;
        --level;
      } else {
        ++level;
      }
    }
    c.prev();
    first_leaf = false;
    if (c.leaf)
      line_offset -= c.leaf->lines();
  }
  return (line > 10 ? line - 10 : 0);
}

std::optional<Iterator> get_hl(const ParserSnapshot &snap, vase::Shard *vase, uint64_t target) {
  if (!snap.root || !snap.lang)
    return std::nullopt;
  return Iterator(target, retain(snap), vase);
}

void Edit::mark_dirty(uint64_t start, uint64_t end) {
  if (!dirty) {
    dirty_start = start;
    dirty_end = end;
    dirty = true;
  } else {
    dirty_start = std::min(dirty_start, start);
    dirty_end = std::max(dirty_end, end);
  }
}

void Edit::insert(uint64_t start, uint64_t count) {
  if (count == 0)
    return;
  uint64_t orig_pos = (uint64_t)((int64_t)start - edit_delta);
  mark_dirty(orig_pos, orig_pos);
  edit_delta += (int64_t)count;
}

void Edit::erase(uint64_t start, uint64_t count) {
  if (count == 0 || !target->root)
    return;
  uint64_t orig_start = (uint64_t)((int64_t)start - edit_delta);
  uint64_t orig_end = orig_start + count;
  mark_dirty(orig_start, orig_end);
  edit_delta -= (int64_t)count;
}

void Edit::commit(vase::Shard *vase) {
  if (!dirty || !target->lang)
    return;
  uint64_t line = dirty_start;
  uint64_t original = dirty_end - dirty_start;
  uint64_t final = (uint64_t)((int64_t)original + edit_delta);
  ParseState *new_root =
    ParseState::splice(*target->lang, target->root, vase, line, original, final);
  release(*target);
  target->root = new_root;
  dirty = false;
  edit_delta = 0;
}

void insert(ParserSnapshot &snap, vase::Shard *vase, uint64_t start, uint64_t count) {
  Edit e(snap);
  e.insert(start, count);
  e.commit(vase);
}

void erase(ParserSnapshot &snap, vase::Shard *vase, uint64_t start, uint64_t count) {
  Edit e(snap);
  e.erase(start, count);
  e.commit(vase);
}
} // namespace bed::internal::syntax
