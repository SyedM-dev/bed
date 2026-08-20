#include "internal/syntax/decl.h"

namespace bed::internal::syntax {
TreeCursor::TreeCursor(ParseState *root, uint64_t target_line, uint64_t *relative) {
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
} // namespace bed::internal::syntax
