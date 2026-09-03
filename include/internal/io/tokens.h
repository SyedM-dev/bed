#pragma once

#include "definitions.h"
#include "pch.h"

namespace bed::internal::io {
struct Token {
  uint32_t start;
  uint32_t end;
  enum Kind : uint8_t {
    TempCurrent,
    AddressSeperator,
    Address,
    Offset,
    AddressRegex,
    AddressSymbol,
    Number,
    Mark,
    RubyFunction,
    RubyArg,
    Function,
    Any,
    Shell,
    Ruby,
    File,
    Regex,
    Replacement,
    Suffix,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Warning,
    Data,
    Shebang,
    Comment,
    Error,
    String,
    Escape,
    Interpolation,
    Regexp,
    True,
    False,
    Char,
    Keyword,
    KeywordOperator,
    Operator,
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

struct Highlight {
  enum : uint8_t {
    None = 0,
    Bold = 1 << 0,
    Italic = 1 << 1,
    Strikethrough = 1 << 2,
    Underline = 1 << 3,
  };
  uint32_t fg;
  uint32_t bg;
  uint8_t flags;
};
} // namespace bed::internal::io
