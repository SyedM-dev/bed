#include "internal/syntax/parser.h"

namespace bed::internal::syntax {
void ParseState::retain(ParseState *n) {
  if (n)
    n->refs++;
};

void ParseState::release(Language &lang, ParseState *n) {
  if (!n || --n->refs > 0)
    return;
  if (n->is_branch()) {
    auto *branch = (ParseStateBranch *)n;
    release(lang, branch->left);
    release(lang, branch->right);
    delete branch;
  } else {
    auto *leaf = (ParseStateLeaf *)n;
    if (leaf->state)
      lang.destroy(leaf->state);
    if (leaf->blocks)
      free(leaf->blocks);
    delete leaf;
  }
}

int height(ParseState *n) {
  return n ? n->height : 0;
}

int balance_factor(ParseState *n) {
  ParseStateBranch *b = (ParseStateBranch *)n;
  return height(b->left) - height(b->right);
}

ParseState *rotate_right(Language &lang, ParseStateBranch *z) {
  ParseStateBranch *y = (ParseStateBranch *)z->left;
  ParseState *middle = new ParseStateBranch(y->right, z->right);
  ParseState *out = new ParseStateBranch(y->left, middle);
  ParseState::release(lang, middle);
  ParseState::release(lang, z);
  return out;
}

ParseState *rotate_left(Language &lang, ParseStateBranch *z) {
  ParseStateBranch *y = (ParseStateBranch *)z->right;
  ParseState *middle = new ParseStateBranch(z->left, y->left);
  ParseState *out = new ParseStateBranch(middle, y->right);
  ParseState::release(lang, middle);
  ParseState::release(lang, z);
  return out;
}

ParseState *balance(Language &lang, ParseState *node) {
  if (!node || !node->is_branch())
    return node;
  ParseStateBranch *b = (ParseStateBranch *)node;
  int bf = balance_factor(node);
  if (bf > 1) {
    ParseStateBranch *left = (ParseStateBranch *)b->left;
    if (balance_factor(left) < 0) {
      ParseState::retain(left);
      auto new_left = rotate_left(lang, left);
      auto rebuilt = new ParseStateBranch(new_left, b->right);
      auto result = rotate_right(lang, (ParseStateBranch *)rebuilt);
      ParseState::release(lang, new_left);
      ParseState::release(lang, b);
      return result;
    }
    return rotate_right(lang, b);
  }
  if (bf < -1) {
    ParseStateBranch *right = (ParseStateBranch *)b->right;
    if (balance_factor(right) > 0) {
      ParseState::retain(right);
      auto new_right = rotate_right(lang, right);
      auto rebuilt = new ParseStateBranch(b->left, new_right);
      auto result = rotate_left(lang, (ParseStateBranch *)rebuilt);
      ParseState::release(lang, new_right);
      ParseState::release(lang, b);
      return result;
    }
    return rotate_left(lang, b);
  }
  return node;
}

ParseState *ParseState::build(Language &lang, ParseState **pieces, uint64_t lo, uint64_t hi) {
  if (hi - lo == 1)
    return pieces[lo];
  uint64_t mid = lo + (hi - lo) / 2;
  ParseState *left = build(lang, pieces, lo, mid);
  ParseState *right = build(lang, pieces, mid, hi);
  ParseState *node = concat(lang, left, right);
  release(lang, left);
  release(lang, right);
  return node;
}

ParseState *ParseState::splice(
  Language &lang, ParseState *root, vase::Shard *vase,
  uint64_t line, uint64_t original, uint64_t final
) {
  if (!vase)
    return nullptr;
  if (!root || (line == 0 && root->lines() == original)) {
    vase::Iterator it(vase, 0, Direction::Forward);
    void *state = lang.none_state();
    std::vector<io::Token> tokens;
    std::vector<ParseEvent> events;
    ParsePieceBuilder builder(lang, 0);
    for (uint64_t at = 0; at < final; ++at) {
      it.next();
      tokens.clear();
      events.clear();
      lang.parse(&state, it.line, at == 0, &tokens, &events);
      builder.add(state, at, events);
    }
    lang.destroy(state);
    return builder.finish();
  }
  uint64_t at;
  void *state;
  ParseState *prefix;
  {
    uint64_t offset;
    TreeCursor c = TreeCursor(lang, root, line, &offset);
    prefix = c.prefix();
    at = line - offset;
    state = lang.copy(c.leaf->state);
  }
  vase::Iterator it(vase, at, Direction::Forward);
  std::vector<io::Token> tokens;
  std::vector<ParseEvent> events;
  uint64_t end_in_tree = line + original;
  uint64_t end_extra;
  TreeCursor c = TreeCursor(lang, root, end_in_tree, &end_extra);
  uint64_t end_in_vase = line + final;
  ParsePieceBuilder builder(lang, at);
  while (at < end_in_vase + end_extra) {
    it.next();
    tokens.clear();
    events.clear();
    lang.parse(&state, it.line, at == 0, &tokens, &events);
    builder.add(state, at, events);
    ++at;
  }
  c.next();
  while (c.leaf) {
    if (lang.equal(state, c.leaf->state))
      break;
    for (uint64_t i = 0; i < c.leaf->lines(); ++i) {
      it.next();
      tokens.clear();
      events.clear();
      lang.parse(&state, it.line, at == 0, &tokens, &events);
      builder.add(state, at, events);
      ++at;
    }
    c.next();
  }
  lang.destroy(state);
  ParseState *suffix = c.suffix();
  ParseState *new_stuff = builder.finish();
  auto a = concat(lang, prefix, new_stuff);
  release(lang, prefix);
  release(lang, new_stuff);
  auto result = concat(lang, a, suffix);
  release(lang, a);
  release(lang, suffix);
  return result;
}

ParseState *ParseState::concat(Language &lang, ParseState *a, ParseState *b) {
  if (!a)
    return (retain(b), b);
  if (!b)
    return (retain(a), a);
  if (a->height > b->height + 1) {
    ParseStateBranch *ba = (ParseStateBranch *)a;
    ParseState *r = concat(lang, ba->right, b);
    ParseState *out = balance(lang, new ParseStateBranch(ba->left, r));
    release(lang, r);
    return out;
  }
  if (b->height > a->height + 1) {
    ParseStateBranch *bb = (ParseStateBranch *)b;
    ParseState *l = concat(lang, a, bb->left);
    ParseState *out = balance(lang, new ParseStateBranch(l, bb->right));
    release(lang, l);
    return out;
  }
  return balance(lang, new ParseStateBranch(a, b));
}
} // namespace bed::internal::syntax
