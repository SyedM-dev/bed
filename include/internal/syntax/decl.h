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
  std::string_view name;
  enum : uint8_t {
    Opening,
    Closing,
    SymbolDef,
    Symbol
  } ev_type;
  uint8_t type; // type.
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

struct ScopeNode {
  static constexpr uint64_t SCOPE_BIT = 1ull << 63;
  static constexpr uint64_t LINES_MASK = ~SCOPE_BIT;
  uint64_t header;
  bool is_scope() const {
    return header & SCOPE_BIT;
  }
  uint64_t lines() const {
    return header & LINES_MASK;
  }
};

struct Symbol {
  uint32_t definition;
  uint16_t reference_count;
  uint8_t type;
  uint8_t len;
  // first chars of count len, padded to 4 bytes.
  // then a set of references. 32bit
  // references later as modifying at end is faster than shifting the name.
};

struct alignas(8) Scope : ScopeNode {
  uint16_t children_count;
  uint16_t symbol_count;
  uint8_t type;
  uint8_t len;
  uint32_t : 32; // if more stuff is needed use the padding first.
  // first chars of count len, padded to 8 bytes.
  // followed by that many number of pointers.
};
} // namespace bed::internal::syntax
