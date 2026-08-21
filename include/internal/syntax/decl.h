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
  enum T : uint8_t {
    Opening,
    Closing,
    SymbolDef,
    Symbol
  } ev_type;
  uint8_t type; // type.

  ParseEvent(T t) : ev_type(t) {}
  ParseEvent(std::string_view s, T e_t, uint8_t type) : name(s), ev_type(e_t), type(type) {}
};

struct Language {
  std::function<void *()> none_state;
  std::function<void(void **, std::string_view, bool, std::vector<Token> *, std::vector<ParseEvent> *)> parse;
  std::function<void *(void *)> copy;
  std::function<bool(void *, void *)> equal;
  std::function<void(void *)> destroy;
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
  uint64_t n;
  static constexpr uint32_t IS_OPENING = 1ull << 31;
  static constexpr uint32_t LINE_MASK = ~IS_OPENING;
  // followed by n number of relative offsets
  // stored as uint32_t with 1 bit for if it is start or end
  // and rest as number.
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
