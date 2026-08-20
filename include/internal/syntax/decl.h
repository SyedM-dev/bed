#pragma once

#include "internal/trie/trie.h"
#include "pch.h"

namespace bed::internal::syntax {
struct Token {
  uint32_t start;
  uint32_t end;
  enum : uint8_t {
    Data,
    Shebang,
    Comment,
    Error,
    String,
    Escape,
    Interpolation,
    Regexp,
    Number,
    True,
    False,
    Char,
    Keyword,
    KeywordOperator,
    Operator,
    Function,
    Namespace,
    Class,
    Module,
    Type,
    Constant,
    VariableInstance,
    VariableGlobal,
    Annotation,
    Directive,
    Label,
    Brace1,
    Brace2,
    Brace3,
    Brace4,
    Brace5,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
    Blockquote,
    List,
    ListItem,
    Code,
    LanguageName,
    LinkLabel,
    ImageLabel,
    Link,
    Table,
    TableHeader,
    Italic,
    Bold,
    Underline,
    Strikethrough,
    HorizontalRule,
    Tag,
    Attribute,
    CheckDone,
    CheckNotDone,
    Count
  } type;
};

struct Language {
  std::function<void *()> none_state;
  std::function<void(void **, std::string_view, bool, std::vector<Token> *)> parse;
  std::function<void *(void *)> copy;
  std::function<bool(void *, void *)> equal;
  std::function<void(void *)> destroy;
};

struct ParseState {
  static constexpr uint64_t BRANCH_BIT = 1ull << 63;
  static constexpr uint64_t LINES_MASK = ~BRANCH_BIT;
  uint64_t header;
  bool is_branch() const {
    return header & BRANCH_BIT;
  }
  uint64_t lines() const {
    return header & LINES_MASK;
  }
};

struct ParseStateBranch : ParseState {
  ParseState *left;
  ParseState *right;
};

struct ParseStateLeaf : ParseState {
  void *state;
};

struct TreeCursor {
  ParseStateLeaf *leaf = nullptr;
  ParseStateBranch *stack[64];
  uint8_t depth = 0;
  bool went_left[64];
  TreeCursor(ParseState *root, uint64_t target_line, uint64_t *relative) {
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
  void next() {
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
  void prev() {
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
};

/*struct Symbol {
  uint64_t definition;
  std::vector<uint64_t> references;
};

struct Space {
  uint64_t len;
};

struct Scope {
  uint64_t len;
  uint32_t type;
  trie::Trie<Symbol> symbols;
  std::vector<std::variant<Space, Scope>> children;
};*/
} // namespace bed::internal::syntax
