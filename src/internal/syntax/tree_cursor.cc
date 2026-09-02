#include "internal/syntax/decl.h"

namespace bed::internal::syntax {
TreeCursor::~TreeCursor() {
  ParseState::release(lang, root);
}

TreeCursor::TreeCursor(
  Language &lang, ParseState *root,
  uint64_t target_line, uint64_t *relative
) : lang(lang), root(root) {
  if (!root) {
    *relative = 0;
    return;
  }
  ParseState::retain(root);
  ParseState *node = root;
  while (node->is_branch()) {
    auto *branch = (ParseStateBranch *)node;
    auto *left = branch->left;
    stack[depth] = branch;
    if (target_line < left->lines()) {
      went_left[depth] = true;
      ++depth;
      node = left;
    } else {
      target_line -= left->lines();
      went_left[depth] = false;
      ++depth;
      node = branch->right;
    }
  }
  *relative = target_line;
  leaf = (ParseStateLeaf *)node;
}

void TreeCursor::next() {
  while (depth > 0) {
    auto *branch = stack[depth - 1];
    bool from_left = went_left[depth - 1];
    --depth;
    if (!from_left)
      continue;
    ParseState *node = branch->right;
    while (node->is_branch()) {
      auto *b = (ParseStateBranch *)node;
      stack[depth] = b;
      went_left[depth] = true;
      ++depth;
      node = b->left;
    }
    leaf = (ParseStateLeaf *)node;
    return;
  }
  leaf = nullptr;
}

void TreeCursor::prev() {
  while (depth > 0) {
    auto *branch = stack[depth - 1];
    bool from_left = went_left[depth - 1];
    --depth;
    if (from_left)
      continue;
    ParseState *node = branch->left;
    while (node->is_branch()) {
      auto *b = (ParseStateBranch *)node;
      stack[depth] = b;
      went_left[depth] = false;
      ++depth;
      node = b->right;
    }
    leaf = (ParseStateLeaf *)node;
    return;
  }
  leaf = nullptr;
}

ParseState *TreeCursor::prefix() {
  ParseState *result = nullptr;
  for (uint8_t i = 0; i < depth; ++i) {
    if (went_left[i])
      continue;
    auto *branch = stack[i];
    ParseState *piece = branch->left;
    if (!result) {
      ParseState::retain(piece);
      result = piece;
    } else {
      ParseState *next = ParseState::concat(lang, result, piece);
      ParseState::release(lang, result);
      result = next;
    }
  }
  return result;
}

ParseState *TreeCursor::suffix() {
  ParseState *result = nullptr;
  for (uint8_t i = depth; i-- > 0;) {
    if (!went_left[i])
      continue;
    auto *branch = stack[i];
    ParseState *piece = branch->right;
    if (!result) {
      ParseState::retain(piece);
      result = piece;
    } else {
      ParseState *next = ParseState::concat(lang, result, piece);
      ParseState::release(lang, result);
      result = next;
    }
  }
  return result;
}
} // namespace bed::internal::syntax
