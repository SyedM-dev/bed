#pragma once

#include "buffer/append.h"
#include "buffer/original.h"
#include "pch.h"
#include "shard.h"

struct Vase {
  OriginalBuffer original;
  AppendBuffer append;

  /*std::vector<Shard *> undo; // TODO: later
  uint8_t top; // of the undo stack.
  uint8_t max; // for redo when no edits have been done after some undo.*/

  Shard *root;

  Vase(char *data, uint32_t length) : original(data, length), append() {
    root = new Petal(length, original.newlines.size(), &original, 0);
  }

  ~Vase() {
    Shard::release(root);
  }

  uint32_t length() {
    return root->length;
  }

  std::string to_string() {
    std::string out;
    flatten(root, out);
    return out;
  }

  void type(uint32_t offset, char key) {
    uint32_t pos = append.key(key);
    Shard *inserted = new Petal(1, key == '\n', &append, pos);
    auto [left, right] = split_shard(root, offset);
    Shard *left2 = append_leaf(left, inserted);
    Shard *new_root = concat_shard(left2, right);
    Shard::release(left);
    Shard::release(right);
    Shard::release(left2);
    Shard::release(inserted);
    Shard::release(root);
    root = new_root;
  }

  void insert(uint32_t offset, const char *data, uint32_t len) {
    uint32_t lines = 0;
    uint32_t pos = append.append(data, len, &lines);
    Shard *inserted = new Petal(len, lines, &append, pos);
    auto [left, right] = split_shard(root, offset);
    Shard *left2 = append_leaf(left, inserted);
    Shard *new_root = concat_shard(left2, right);
    Shard::release(left);
    Shard::release(right);
    Shard::release(left2);
    Shard::release(inserted);
    Shard::release(root);
    root = new_root;
  }

  void erase(uint32_t cursor, int64_t amount) {
    if (amount == 0)
      return;
    uint32_t start;
    uint32_t count;
    if (amount < 0) {
      count = std::min<uint32_t>(-amount, cursor);
      start = cursor - count;
    } else {
      start = cursor;
      count = amount;
    }
    auto [a, b] = split_shard(root, start);
    auto [d, c] = split_shard(b, count);
    Shard *new_root = concat_shard(a, c);
    Shard::release(a);
    Shard::release(b);
    Shard::release(c);
    Shard::release(d);
    Shard::release(root);
    root = new_root;
  }

  void flatten(Shard *s, std::string &out) {
    if (s->kind == Shard::ShardKind::Petal) {
      auto *p = static_cast<Petal *>(s);
      uint32_t remaining = p->length;
      uint32_t pos = p->pos;
      while (remaining) {
        uint32_t got;
        const char *data = p->source->read(pos, &got);
        uint32_t take = std::min(got, remaining);
        out.append(data, take);
        remaining -= take;
        pos += take;
      }
    } else {
      auto *b = static_cast<Branch *>(s);
      flatten(b->left, out);
      flatten(b->right, out);
    }
  }
};
