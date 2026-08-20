#include "internal/syntax/parser.h"

namespace bed::internal::syntax {
static void destroy_tree(ParseState *node, Language &lang) {
  if (!node)
    return;
  if (node->is_branch()) {
    auto *branch = (ParseStateBranch *)node;
    destroy_tree(branch->left, lang);
    destroy_tree(branch->right, lang);
    delete branch;
  } else {
    auto *leaf = (ParseStateLeaf *)node;
    if (leaf->state)
      lang.destroy(leaf->state);
    delete leaf;
  }
}

static ParseState *make_branch(ParseState *left, ParseState *right) {
  if (!left)
    return right;
  if (!right)
    return left;
  auto *branch = new ParseStateBranch;
  branch->header =
    ParseState::BRANCH_BIT + left->lines() + right->lines();
  branch->left = left;
  branch->right = right;
  return branch;
}

static ParseState *build_tree(std::vector<ParseStateLeaf *> &leaves, size_t begin, size_t end) {
  const size_t count = end - begin;
  if (count == 0)
    return nullptr;
  if (count == 1)
    return leaves[begin];
  const size_t mid = begin + count / 2;
  ParseState *left = build_tree(leaves, begin, mid);
  ParseState *right = build_tree(leaves, mid, end);
  return make_branch(left, right);
}

Parser::Parser(vase::Vase &vase, uint64_t lines, Language lang)
    : root(nullptr), lang(lang) {
  reset(vase, lines, lang);
}

Parser::~Parser() {
  destroy_tree(root, lang);
}

void Parser::reset(vase::Vase &vase, uint64_t lines, Language lang_) {
  destroy_tree(root, lang);
  root = nullptr;
  if (lines == 0)
    return;
  lang = std::move(lang_);
  vase::Iterator it = vase.iterate(0, Direction::Forward);
  it.next();
  std::vector<ParseStateLeaf *> leaves;
  std::vector<Token> tokens;
  leaves.reserve((lines + MAX_CHUNK - 1) / MAX_CHUNK);
  void *state = lang.none_state();
  uint32_t consumed = 0;
  while (consumed < lines) {
    ParseStateLeaf *leaf = new ParseStateLeaf;
    leaf->header = 0;
    leaf->state = lang.copy(state);
    uint32_t chunk_lines = 0;
    while (chunk_lines < MAX_CHUNK && consumed < lines) {
      tokens.clear();
      lang.parse(&state, it.line, consumed == 0, &tokens);
      ++chunk_lines;
      ++consumed;
      if (!it.next() && consumed < lines)
        break;
    }
    leaf->header = chunk_lines;
    leaves.push_back(leaf);
  }
  lang.destroy(state);
  root = build_tree(leaves, 0, leaves.size());
}

std::pair<ParseState *, ParseState *> Parser::split_tree(ParseState *node, uint64_t line) {
  if (!node)
    return {nullptr, nullptr};
  if (line == 0)
    return {nullptr, node};
  if (line >= node->lines())
    return {node, nullptr};
  if (node->is_branch()) {
    auto *branch = (ParseStateBranch *)node;
    uint64_t left_lines = branch->left->lines();
    if (line < left_lines) {
      auto [a, b] = split_tree(branch->left, line);
      ParseState *right = join_tree(b, branch->right);
      delete branch;
      return {a, right};
    }
    if (line == left_lines) {
      ParseState *left = branch->left;
      ParseState *right = branch->right;
      delete branch;
      return {left, right};
    }
    auto [a, b] = split_tree(branch->right, line - left_lines);
    ParseState *left = join_tree(branch->left, a);
    delete branch;
    return {left, b};
  } else {
    auto *leaf = (ParseStateLeaf *)node;
    uint64_t lines = leaf->lines();
    auto *right = new ParseStateLeaf;
    right->header = lines - line;
    right->state = nullptr;
    leaf->header = line;
    return {leaf, right};
  }
}

ParseState *Parser::join_tree(ParseState *a, ParseState *b) {
  // TODO: balance
  return make_branch(a, b);
}

void Parser::erase(vase::Vase &vase, uint64_t start, uint64_t count) {
  if (count == 0 || !root)
    return;
  auto [a, remaining] = split_tree(root, start);
  auto [waste, b] = split_tree(remaining, count);
  destroy_tree(waste, lang);
  root = join_tree(a, b);
  modify(vase, start, 1);
}

void Parser::insert(vase::Vase &vase, uint64_t start, uint64_t count) {
  if (count == 0)
    return;
  std::vector<ParseStateLeaf *> leaves;
  leaves.reserve((count + MAX_CHUNK - 1) / MAX_CHUNK);
  uint64_t consumed = 0;
  while (consumed < count) {
    auto *leaf = new ParseStateLeaf;
    leaf->state = nullptr;
    uint64_t chunk = 0;
    while (chunk < MAX_CHUNK && consumed < count) {
      ++chunk;
      ++consumed;
    }
    leaf->header = chunk;
    leaves.push_back(leaf);
  }
  ParseState *subtree = build_tree(leaves, 0, leaves.size());
  auto [left, right] = split_tree(root, start);
  root = join_tree(join_tree(left, subtree), right);
  modify(vase, start, count);
}

void Parser::modify(vase::Vase &vase, uint64_t target, uint64_t count) {
  if (count == 0 || !root)
    return;
  std::vector<Token> tokens;
  uint64_t offset;
  TreeCursor c = TreeCursor(root, target, &offset);
  uint64_t at = target - offset;
  void *state = nullptr;
  if (c.leaf->state) {
    state = lang.copy(c.leaf->state);
  } else {
    while (!c.leaf->state) {
      c.prev();
      if (!c.leaf) {
        at = 0;
        break;
      }
      at -= c.leaf->lines();
    }
    if (c.leaf) {
      state = lang.copy(c.leaf->state);
    } else {
      state = lang.none_state();
      c = TreeCursor(root, 0, &offset);
    }
  }
  vase::Iterator it = vase.iterate(at, Direction::Forward);
  uint64_t next_boundary = at + c.leaf->lines();
  while (true) {
    it.next();
    if (at == next_boundary) {
      c.next();
      if (!c.leaf)
        break;
      next_boundary += c.leaf->lines();
      if (at >= target + count
          && c.leaf->state != nullptr
          && lang.equal(state, c.leaf->state))
        break;
      if (c.leaf->state)
        lang.destroy(c.leaf->state);
      c.leaf->state = lang.copy(state);
    }
    tokens.clear();
    lang.parse(&state, it.line, at == 0, &tokens);
    at++;
  }
  lang.destroy(state);
}

std::optional<Parser::Iterator> Parser::get_hl(vase::Vase &vase, uint64_t target) {
  if (!root)
    return std::nullopt;
  return Parser::Iterator(target, this, vase);
}

Parser::Iterator::Iterator(uint64_t target, Parser *p, vase::Vase &vase) : p(p) {
  uint64_t offset;
  TreeCursor c = TreeCursor(p->root, target, &offset);
  at = target - offset;
  if (c.leaf->state) {
    state = p->lang.copy(c.leaf->state);
  } else {
    while (!c.leaf->state) {
      c.prev();
      if (!c.leaf) {
        at = 0;
        break;
      }
      at -= c.leaf->lines();
    }
    if (c.leaf) {
      state = p->lang.copy(c.leaf->state);
    } else {
      state = p->lang.none_state();
      c = TreeCursor(p->root, 0, &offset);
    }
  }
  it = vase.iterate(at, Direction::Forward);
  while (at < target) {
    it->next();
    tokens.clear();
    p->lang.parse(&state, it->line, at == 0, &tokens);
    at++;
  }
}

Parser::Iterator::~Iterator() {
  if (state)
    p->lang.destroy(state);
}

Parser::Iterator::Iterator(Iterator &&other)
    : p(other.p),
      it(std::move(other.it)),
      state(other.state),
      tokens(std::move(other.tokens)) {
  other.state = nullptr;
}

Parser::Iterator &Parser::Iterator::operator=(Iterator &&other) {
  if (this == &other)
    return *this;
  if (state)
    p->lang.destroy(state);
  p = other.p;
  it = std::move(other.it);
  state = other.state;
  tokens = std::move(other.tokens);
  other.state = nullptr;
  return *this;
}

void Parser::Iterator::next() {
  it->next();
  tokens.clear();
  p->lang.parse(&state, it->line, at++ == 0, &tokens);
}
} // namespace bed::internal::syntax
