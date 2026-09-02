#pragma once

#include "internal/trie/trie.h"
#include "internal/vase/vase.h"
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
  std::function<void(
    void **, std::string_view,
    bool, std::vector<Token> *, std::vector<ParseEvent> *
  )>
    parse;
  std::function<void *(void *)> copy;
  std::function<bool(void *, void *)> equal;
  std::function<void(void *)> destroy;
};

struct ParseState {
  static constexpr uint64_t BRANCH_BIT = 1ull << 63;
  static constexpr uint64_t LINES_MASK = ~BRANCH_BIT;
  uint64_t header;
  uint16_t height;
  std::atomic_uint16_t refs;
  bool is_branch() const {
    return header & BRANCH_BIT;
  }
  uint64_t lines() const {
    return header & LINES_MASK;
  }
  explicit ParseState(bool branch, uint64_t lines, uint16_t height)
      : height(height), refs(1) {
    header = lines;
    header |= branch * BRANCH_BIT;
  };
  static void retain(ParseState *node);
  static void release(Language &lang, ParseState *node);
  static ParseState *build(Language &lang, ParseState **pieces, uint64_t lo, uint64_t hi);
  static ParseState *splice(
    Language &lang, ParseState *node, vase::Shard *vase,
    uint64_t line, uint64_t original, uint64_t final
  );
  static ParseState *concat(Language &lang, ParseState *a, ParseState *b);
};

struct ParseStateBranch : ParseState {
  ParseState *left;
  ParseState *right;
  ParseStateBranch(ParseState *l, ParseState *r)
      : ParseState(
          true, l->lines() + r->lines(),
          1 + std::max(l->height, r->height)
        ),
        left(l), right(r) {
    retain(l);
    retain(r);
  }
};

struct ParseStateLeaf : ParseState {
  static constexpr uint64_t MAX_CHUNK = 512;
  uint32_t n{0};
  static constexpr uint16_t IS_CLOSING = 0x8000;
  static constexpr uint16_t LINE_MASK = 0x7fff;
  uint16_t *blocks{nullptr};
  void *state;
  ParseStateLeaf(void *state, uint64_t lines, uint32_t n, uint16_t *blocks_)
      : ParseState(false, lines, 1), n(n), state(state) {
    blocks = (uint16_t *)malloc(sizeof(uint16_t) * n);
    memcpy(blocks, blocks_, sizeof(uint16_t) * n);
  };
};

struct ParsePieceBuilder {
  Language &lang;
  std::vector<ParseState *> pieces;
  std::vector<uint16_t> blocks;
  void *piece_state{nullptr};
  uint64_t chunk_start{0};
  uint64_t chunk_lines{0};
  ParsePieceBuilder(Language &lang, uint64_t first_line)
      : lang(lang), chunk_start(first_line) {}
  void add(
    void *state,
    uint64_t line,
    const std::vector<ParseEvent> &events
  ) {
    if (chunk_lines == 0) {
      chunk_start = line;
      piece_state = lang.copy(state);
    }
    for (const auto &ev : events) {
      blocks.push_back(
        (ev.closing ? ParseStateLeaf::IS_CLOSING : 0)
        | (line - chunk_start)
      );
    }
    ++chunk_lines;
    if (chunk_lines == ParseStateLeaf::MAX_CHUNK)
      flush();
  }
  void flush() {
    if (chunk_lines == 0)
      return;
    pieces.push_back(new ParseStateLeaf(
      piece_state,
      chunk_lines,
      blocks.size(),
      blocks.data()
    ));
    piece_state = nullptr;
    blocks.clear();
    chunk_lines = 0;
  }
  ParseState *finish() {
    flush();
    if (pieces.empty())
      return nullptr;
    ParseState *root = ParseState::build(lang, pieces.data(), 0, pieces.size());
    pieces.clear();
    return root;
  }
};

struct TreeCursor {
  Language &lang;
  ParseState *root;
  ParseStateLeaf *leaf = nullptr;
  ParseStateBranch *stack[64];
  uint8_t depth = 0;
  bool went_left[64];
  TreeCursor(
    Language &lang, ParseState *root,
    uint64_t target_line, uint64_t *relative
  );
  ~TreeCursor();
  TreeCursor(const TreeCursor &) = delete;
  TreeCursor &operator=(const TreeCursor &) = delete;
  void next();
  void prev();
  ParseState *prefix();
  ParseState *suffix();
};
} // namespace bed::internal::syntax
