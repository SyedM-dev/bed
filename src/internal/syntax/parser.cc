#include "internal/syntax/parser.h"

namespace bed::internal::syntax {
void dump_events(ParseState *root) {
  if (!root)
    return;
  uint64_t offset = 0;
  TreeCursor c(root, 0, &offset);
  uint64_t line_offset = 0;
  int depth = 0;
  while (c.leaf) {
    auto *leaf = c.leaf;
    for (uint32_t i = 0; i < leaf->n; ++i) {
      uint32_t block = leaf->blocks[i];
      uint32_t pos = block & ParseStateLeaf::LINE_MASK;
      bool closing = block & ParseStateLeaf::IS_CLOSING;
      if (closing) {
        if (depth > 0)
          --depth;
        else
          std::cout << "!! UNMATCHED CLOSE !! ";
      }
      std::cout << std::string(static_cast<size_t>(depth) * 2, ' ')
                << (closing ? "}" : "{")
                << " line " << (line_offset + pos)
                << '\n';
      if (!closing)
        ++depth;
    }
    line_offset += leaf->lines();
    c.next();
  }
  if (depth != 0) {
    std::cout << "!! UNBALANCED: depth = "
              << depth
              << " !!\n";
  }
}

static void destroy_tree(ParseState *node, Language &lang) {
  if (!node)
    return;
  if (node->is_branch()) {
    auto *branch = (ParseStateBranch *)node;
    destroy_tree(branch->left, lang);
    destroy_tree(branch->right, lang);
    free(branch);
  } else {
    auto *leaf = (ParseStateLeaf *)node;
    if (leaf->state)
      lang.destroy(leaf->state);
    if (leaf->blocks)
      free(leaf->blocks);
    free(leaf);
  }
}

static ParseState *make_branch(ParseState *left, ParseState *right) {
  if (!left)
    return right;
  if (!right)
    return left;
  auto *branch = (ParseStateBranch *)malloc(sizeof(ParseStateBranch));
  branch->header = ParseState::BRANCH_BIT + left->lines() + right->lines();
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
  std::vector<ParseStateLeaf *> leaves;
  leaves.reserve((lines + MAX_CHUNK - 1) / MAX_CHUNK);
  uint64_t consumed = 0;
  while (consumed < lines) {
    auto *leaf = (ParseStateLeaf *)malloc(sizeof(ParseStateLeaf));
    leaf->state = nullptr;
    leaf->blocks = nullptr;
    leaf->n = 0;
    leaf->cap = 0;
    uint64_t chunk = std::min(MAX_CHUNK, lines - consumed);
    leaf->header = chunk;
    consumed += chunk;
    leaves.push_back(leaf);
  }
  root = build_tree(leaves, 0, leaves.size());
  modify(vase, 0, lines);
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
      free(branch);
      return {a, right};
    }
    if (line == left_lines) {
      ParseState *left = branch->left;
      ParseState *right = branch->right;
      free(branch);
      return {left, right};
    }
    auto [a, b] = split_tree(branch->right, line - left_lines);
    ParseState *left = join_tree(branch->left, a);
    free(branch);
    return {left, b};
  } else {
    auto *leaf = (ParseStateLeaf *)node;
    uint64_t lines = leaf->lines();
    auto *right = (ParseStateLeaf *)malloc(sizeof(ParseStateLeaf));
    right->header = lines - line;
    right->state = nullptr;
    right->blocks = nullptr;
    right->n = 0;
    right->cap = 0;
    leaf->header = line;
    leaf->n = 0;
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
    auto *leaf = (ParseStateLeaf *)malloc(sizeof(ParseStateLeaf));
    leaf->state = nullptr;
    leaf->blocks = nullptr;
    leaf->n = 0;
    leaf->cap = 0;
    uint64_t chunk = std::min(MAX_CHUNK, count - consumed);
    leaf->header = chunk;
    consumed += chunk;
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
  std::vector<ParseEvent> events;
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
  uint64_t chunk_start = at;
  uint64_t next_boundary = at + c.leaf->lines();
  c.leaf->n = 0;
  while (true) {
    it.next();
    if (at == next_boundary) {
      c.next();
      if (!c.leaf)
        break;
      c.leaf->n = 0;
      chunk_start = at;
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
    events.clear();
    lang.parse(&state, it.line, at == 0, &tokens, &events);
    for (const auto &ev : events) {
      const auto t = uint8_t(ev.ev_type);
      if (t > ParseEvent::Closing)
        continue;
      if (c.leaf->n == c.leaf->cap) {
        uint32_t cap = c.leaf->cap ? c.leaf->cap * 2 : 8;
        c.leaf->blocks = (uint32_t *)realloc(c.leaf->blocks, cap * sizeof(uint32_t));
        c.leaf->cap = cap;
      }
      c.leaf->blocks[c.leaf->n++] = t << 31 | (at - chunk_start);
    }
    at++;
  }
  lang.destroy(state);
}

uint64_t Parser::next_closing(uint64_t line) {
  if (!root)
    return line + 10;
  uint64_t relative;
  TreeCursor c(root, line, &relative);
  uint64_t at = line - relative;
  uint32_t from = (uint32_t)relative;
  bool first = true;
  int depth = 0;
  while (c.leaf) {
    auto *leaf = c.leaf;
    for (uint32_t i = 0; i < leaf->n; ++i) {
      uint32_t block = leaf->blocks[i];
      uint32_t pos = block & ParseStateLeaf::LINE_MASK;
      if (first && pos <= from)
        continue;
      if (block & ParseStateLeaf::IS_CLOSING) {
        if (depth == 0)
          return at + pos;
        --depth;
      } else {
        ++depth;
      }
    }
    at += leaf->lines();
    first = false;
    c.next();
  }
  return line + 10;
}

uint64_t Parser::prev_opening(uint64_t line) {
  if (!root)
    return line >= 10 ? line - 10 : 0;
  uint64_t relative;
  TreeCursor c(root, line, &relative);
  uint64_t at = line - relative;
  uint32_t from = (uint32_t)relative;
  bool first = true;
  int depth = 0;
  while (c.leaf) {
    auto *leaf = c.leaf;
    for (uint32_t i = leaf->n; i-- > 0;) {
      uint32_t block = leaf->blocks[i];
      uint32_t pos = block & ParseStateLeaf::LINE_MASK;
      if (first && pos >= from)
        continue;
      if (!(block & ParseStateLeaf::IS_CLOSING)) {
        if (depth == 0)
          return at + pos;
        --depth;
      } else {
        ++depth;
      }
    }
    c.prev();
    if (!c.leaf)
      break;
    at -= c.leaf->lines();
    first = false;
  }
  return line >= 10 ? line - 10 : 0;
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
    events.clear();
    p->lang.parse(&state, it->line, at == 0, &tokens, &events);
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
  events.clear();
  p->lang.parse(&state, it->line, at++ == 0, &tokens, &events);
}
} // namespace bed::internal::syntax
