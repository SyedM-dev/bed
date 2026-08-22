#pragma once

#include "internal/trie/trie.h"
#include "pch.h"

namespace bed::internal::syntax {
struct Token {
  uint32_t start;
  uint32_t end;
  enum Kind : uint8_t {
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

struct ParseEvent {
  uint8_t closing;
  ParseEvent(uint8_t t) : closing(t) {}
};

struct Language {
  std::function<void *()> none_state;
  std::function<void(void **, std::string_view, bool, std::vector<Token> *, std::vector<ParseEvent> *)> parse;
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
  uint32_t n;
  uint32_t cap;
  uint16_t *blocks;
  static constexpr uint16_t IS_CLOSING = 0x8000;
  static constexpr uint16_t LINE_MASK = 0x7fff;
};

struct TreeCursor {
  ParseStateLeaf *leaf = nullptr;
  ParseStateBranch *stack[64];
  uint8_t depth = 0;
  bool went_left[64];
  TreeCursor(ParseState *root, uint64_t target_line, uint64_t *relative);
  void next();
  void prev();
};
} // namespace bed::internal::syntax
