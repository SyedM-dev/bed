#include "internal/syntax/ruby/parser.h"

namespace bed::internal::syntax::ruby {
inline bool is_hex(char c) {
  return ('0' <= c && c <= '9')
         || ('a' <= c && c <= 'f')
         || ('A' <= c && c <= 'F');
};

inline bool identifier_start_char(char c) {
  return (c & 0x80)
         || ('a' <= c && c <= 'z')
         || ('A' <= c && c <= 'Z')
         || c == '_';
}

inline bool identifier_char(char c) {
  return (c & 0x80)
         || ('a' <= c && c <= 'z')
         || ('A' <= c && c <= 'Z')
         || ('0' <= c && c <= '9')
         || c == '_';
}

inline uint8_t utf8_codepoint_width(unsigned char c) {
  if ((c & 0x80) == 0x00)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 1;
}

bool handle_escapes(RubyParser &p, std::vector<Token> *tokens, uint32_t &start, bool string = true) {
  if (p.peek() == '\\') {
    if (string)
      tokens->push_back({start, p.i, Token::String});
    else
      tokens->push_back({start, p.i, Token::Regexp});
    start = p.i;
    p.advance();
    if (p.peek() == 'x') {
      p.advance();
      if (is_hex(p.peek()))
        p.advance();
      if (is_hex(p.peek()))
        p.advance();
    } else if (p.peek() == 'u') {
      p.advance();
      if (p.peek() == '{') {
        p.advance();
        while (p.peek() != '}' && p.peek() != '\0')
          p.advance();
        if (p.peek() == '}')
          p.advance();
      } else {
        if (is_hex(p.peek()))
          p.advance();
        if (is_hex(p.peek()))
          p.advance();
        if (is_hex(p.peek()))
          p.advance();
        if (is_hex(p.peek()))
          p.advance();
      }
    } else if ('0' <= p.peek() && p.peek() <= '7') {
      p.advance();
      if ('0' <= p.peek() && p.peek() <= '7')
        p.advance();
      if ('0' <= p.peek() && p.peek() <= '7')
        p.advance();
    } else if (p.peek() == 'c') {
      p.advance();
      if (p.peek() != '\\')
        p.advance();
    } else if (p.peek() == 'M' || p.peek() == 'C') {
      p.advance();
      if (p.peek() == '-') {
        p.advance();
        if (p.peek() != '\\')
          p.advance();
      }
    } else if (p.peek() == 'N') {
      p.advance();
      if (p.peek() == '{') {
        p.advance();
        while (p.peek() != '}' && p.peek() != '\0')
          p.advance();
        if (p.peek() == '}')
          p.advance();
      }
    } else {
      p.advance();
    }
    tokens->push_back({start, p.i, Token::Escape});
    start = p.i;
    return true;
  }
  return false;
};

bool handle_heredoc(RubyParser &p, std::vector<Token> *tokens) {
  uint8_t *heredocs = p.state->heredocs();
  uint32_t start = p.i;
  if (start == 0) {
    uint32_t heredoc_len = heredocs[0] & RubyState::Heredocs::LEN_MASK;
    if (heredocs[0] & RubyState::Heredocs::ALLOW_INDENTATION)
      while (start < p.len() && (p.line[start] == ' ' || p.line[start] == '\t'))
        start++;
    if (p.len() - start == heredoc_len
        && memcmp(p.line.data() + start, heredocs + 1, heredoc_len) == 0) {
      if (!p.dequeue_doc(heredoc_len))
        p.current().state = RubyState::RubyInternalState::NONE;
      tokens->push_back({p.i, p.len(), Token::Annotation});
      return true;
    }
  }
  if (!(heredocs[0] & RubyState::Heredocs::ALLOW_INTERPOLATION)) {
    tokens->push_back({p.i, p.len(), Token::String});
    return true;
  } else {
    while (p.i < p.len()) {
      if (handle_escapes(p, tokens, start))
        continue;
      if (p.peek_str(2) == "#{") {
        tokens->push_back({start, p.i, Token::String});
        tokens->push_back({p.i, p.i + 2, Token::Interpolation});
        p.advance(2);
        p.push_state();
        break;
      }
      p.advance();
    }
    if (p.i >= p.len())
      tokens->push_back({start, p.len(), Token::String});
    return false;
  }
}

void handle_string(RubyParser &p, std::vector<Token> *tokens) {
  uint32_t start = p.i;
  while (p.i < p.len()) {
    if (handle_escapes(p, tokens, start))
      continue;
    if ((p.current().flags & RubyState::RubyInternalState::ALLOW_INTERPOLATION)
        && p.peek_str(2) == "#{") {
      tokens->push_back({start, p.i, Token::String});
      tokens->push_back({p.i, p.i + 2, Token::Interpolation});
      p.advance(2);
      p.push_state();
      break;
    }
    if (p.peek() == p.current().delim_start
        && p.current().delim_start != p.current().delim_end)
      p.current().lit_brace_level++;
    if (p.peek() == p.current().delim_end) {
      if (p.current().delim_start == p.current().delim_end) {
        p.advance();
        tokens->push_back({start, p.i, Token::String});
        p.current().state = RubyState::RubyInternalState::NONE;
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        return;
      } else {
        p.current().lit_brace_level--;
        if (p.current().lit_brace_level == 0) {
          p.advance();
          tokens->push_back({start, p.i, Token::String});
          p.current().state = RubyState::RubyInternalState::NONE;
          p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
          return;
        }
      }
    }
    p.advance();
  }
  if (p.i >= p.len())
    tokens->push_back({start, p.len(), Token::String});
}

void handle_regex(RubyParser &p, std::vector<Token> *tokens) {
  uint32_t start = p.i;
  while (p.i < p.len()) {
    if (handle_escapes(p, tokens, start, false))
      continue;
    if ((p.current().flags & RubyState::RubyInternalState::ALLOW_INTERPOLATION)
        && p.peek_str(2) == "#{") {
      tokens->push_back({start, p.i, Token::Regexp});
      tokens->push_back({p.i, p.i + 2, Token::Interpolation});
      p.advance(2);
      p.push_state();
      break;
    }
    if (p.peek() == p.current().delim_start
        && p.current().delim_start != p.current().delim_end)
      p.current().lit_brace_level++;
    if (p.peek() == p.current().delim_end) {
      if (p.current().delim_start == p.current().delim_end) {
        p.advance();
        tokens->push_back({start, p.i, Token::Regexp});
        p.current().state = RubyState::RubyInternalState::NONE;
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        return;
      } else {
        p.current().lit_brace_level--;
        if (p.current().lit_brace_level == 0) {
          p.advance();
          tokens->push_back({start, p.i, Token::Regexp});
          p.current().state = RubyState::RubyInternalState::NONE;
          p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
          return;
        }
      }
    }
    p.advance();
  }
  if (p.i >= p.len())
    tokens->push_back({start, p.len(), Token::Regexp});
}

bool handle_line_markers(RubyParser &p, std::vector<Token> *tokens, std::vector<ParseEvent> *events) {
  if (p.len() == 6 && p.peek_str(6) == "=begin") {
    p.current().state = RubyState::RubyInternalState::COMMENT;
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    events->push_back({p.peek_str(6), ParseEvent::Opening, (uint8_t)ScopeTypes::Comment});
    tokens->push_back({0, p.len(), Token::Comment});
    return true;
  }
  if (p.len() == 7 && p.peek_str(7) == "__END__") {
    p.current().state = RubyState::RubyInternalState::END;
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    return true;
  }
  return false;
}

bool handle_comment(RubyParser &p, std::vector<Token> *tokens, bool first_line) {
  if (p.peek() == '#') {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    if (first_line && p.i == 0 && p.peek(1) == '!') {
      tokens->push_back({0, p.len(), Token::Shebang});
      return true;
    }
    tokens->push_back({p.i, p.len(), Token::Comment});
    return true;
  }
  return false;
}

void handle_syntax(RubyParser &p, std::vector<Token> *tokens, std::vector<ParseEvent> *events) {
  static const RubyTries tries = RubyTries();
  if (p.current().flags & RubyState::RubyInternalState::NAME_MASK) {
    switch (p.current().flags & RubyState::RubyInternalState::NAME_MASK) {
    case RubyState::RubyInternalState::CLASS_NAME: {
      while (p.peek() == ' ' || p.peek() == '\t')
        p.advance();
      if (p.peek() == '\0')
        return;
      if (p.peek() == '<' && p.peek(1) == '<') {
        p.advance(2);
        events->push_back({"singleton", ParseEvent::Opening, (uint8_t)ScopeTypes::Class});
        p.current().flags = (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK);
        p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        return;
      }
      uint32_t k = p.i;
      uint32_t j = 0;
      if (identifier_start_char(p.peek(j))) {
        j++;
        while (identifier_char(p.peek(j)))
          j++;
        tokens->push_back({p.i, p.i + j, Token::Constant});
        p.advance(j);
        while (p.peek() == ' ' || p.peek() == '\t')
          p.advance();
        if (p.peek() == ':' && p.peek(1) == ':') {
          p.advance(2);
          return;
        }
      }
      events->push_back({p.line.substr(k, j), ParseEvent::Opening, (uint8_t)ScopeTypes::Class});
      p.current().flags = (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK);
      return;
    }
    case RubyState::RubyInternalState::DEF_NAME: {
      while (p.peek() == ' ' || p.peek() == '\t')
        p.advance();
      if (p.peek() == '\0')
        return;
      uint32_t k = p.i;
      uint32_t j = 0;
      if (identifier_start_char(p.peek(j))) {
        j++;
        while (identifier_char(p.peek(j)))
          j++;
        if (p.peek(j) == '!' || p.peek(j) == '?')
          j++;
        if ('A' <= p.peek() && p.peek() <= 'Z')
          tokens->push_back({p.i, p.i + j, Token::Constant});
        else if (j == 4 && p.peek_str(4) == "self")
          tokens->push_back({p.i, p.i + j, Token::Keyword});
        else
          tokens->push_back({p.i, p.i + j, Token::Function});
        p.advance(j);
        while (p.peek() == ' ' || p.peek() == '\t')
          p.advance();
        if (p.peek() == '.') {
          p.advance();
          return;
        }
      }
      events->push_back({p.line.substr(k, j), ParseEvent::Opening, (uint8_t)ScopeTypes::Method});
      p.current().flags = (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK);
      return;
    }
    case RubyState::RubyInternalState::MODULE_NAME: {
      while (p.peek() == ' ' || p.peek() == '\t')
        p.advance();
      if (p.peek() == '\0')
        return;
      uint32_t k = p.i;
      uint32_t j = 0;
      if (identifier_start_char(p.peek(j))) {
        j++;
        while (identifier_char(p.peek(j)))
          j++;
        tokens->push_back({p.i, p.i + j, Token::Constant});
        p.advance(j);
        while (p.peek() == ' ' || p.peek() == '\t')
          p.advance();
        if (p.peek() == ':' && p.peek(1) == ':') {
          p.advance(2);
          return;
        }
      }
      events->push_back({p.line.substr(k, j), ParseEvent::Opening, (uint8_t)ScopeTypes::Module});
      p.current().flags = (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK);
      return;
    }
    }
  }
  if (p.i + 3 <= p.len() && p.peek_str(2) == "<<") {
    uint32_t j = 2;
    bool indented = false;
    if (p.peek(j) == '~')
      indented = true;
    if (p.peek(j) == '~' || p.peek(j) == '-')
      j++;
    tokens->push_back({p.i, p.i + j, Token::Operator});
    if (j >= p.len())
      return;
    std::string delim;
    bool interpolation = true;
    uint32_t s = p.i + j;
    if (p.peek(j) == '\'' || p.peek(j) == '"') {
      char q = p.peek(j++);
      if (q == '\'')
        interpolation = false;
      while (j < p.len() && p.peek(j) != q)
        delim += p.peek(j++);
    } else {
      if (j < p.len() && identifier_start_char(p.peek(j))) {
        delim += p.peek(j++);
        while (j < p.len() && identifier_char(p.peek(j)))
          delim += p.peek(j++);
      }
    }
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    if (!delim.empty()) {
      tokens->push_back({s, p.i + j, Token::Annotation});
      uint8_t header = delim.size();
      if (interpolation)
        header |= RubyState::Heredocs::ALLOW_INTERPOLATION;
      if (indented)
        header |= RubyState::Heredocs::ALLOW_INDENTATION;
      p.enqueue_doc(header, delim);
      p.current().state = RubyState::RubyInternalState::HEREDOC;
      p.heredoc_start_line = true;
    }
    p.advance(j);
    return;
  }
  if (p.peek() == '/' && p.current().flags & RubyState::RubyInternalState::EXPECTING_EXPRESSION) {
    tokens->push_back({p.i, p.i + 1, Token::Regexp});
    p.current().state = RubyState::RubyInternalState::REGEXP;
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    p.current().flags |= RubyState::RubyInternalState::ALLOW_INTERPOLATION;
    p.current().delim_start = '/';
    p.current().delim_end = '/';
    p.advance();
    return;
  }
  switch (p.peek()) {
  case '.': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint32_t start = p.i;
    p.advance();
    if (p.peek() == '.') {
      p.advance();
      if (p.peek() == '.')
        p.advance();
    }
    tokens->push_back({start, p.i, Token::Operator});
    return;
  }
  case ':': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint32_t start = p.i;
    p.advance();
    if (p.i >= p.len()) {
      tokens->push_back({start, p.i, Token::Operator});
      p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      return;
    }
    if (p.peek() == ':') {
      p.advance();
      tokens->push_back({start, p.i, Token::Operator});
      return;
    }
    if (p.peek() == '\'' || p.peek() == '"') {
      tokens->push_back({start, p.i, Token::Label});
      p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      return;
    }
    if (p.peek() == '$' || p.peek() == '@') {
      if (p.peek_str(2) == "@@")
        p.advance(2);
      else
        p.advance();
      while (identifier_char(p.peek()))
        p.advance();
      tokens->push_back({start, p.i, Token::Label});
      return;
    }
    uint32_t op_len = tries.operator_trie.longest_match(p.peek_str(p.len() - p.i));
    if (op_len > 0) {
      tokens->push_back({start, p.i + op_len, Token::Label});
      p.advance(op_len);
      return;
    }
    if (identifier_start_char(p.peek())) {
      p.advance();
      while (identifier_char(p.peek()))
        p.advance();
      if (p.peek() == '!' || p.peek() == '?')
        p.advance();
      tokens->push_back({start, p.i, Token::Label});
      return;
    }
    tokens->push_back({start, p.i, Token::Operator});
    return;
  }
  case '@': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint32_t start = p.i;
    p.advance();
    if (p.i >= p.len())
      return;
    if (p.peek() == '@')
      p.advance();
    if (identifier_start_char(p.peek()))
      p.advance();
    else
      return;
    while (identifier_char(p.peek()))
      p.advance();
    tokens->push_back({start, p.i, Token::VariableInstance});
    return;
  }
  case '$': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint32_t start = p.i;
    p.advance();
    if (p.i >= p.len())
      return;
    if (identifier_start_char(p.peek())) {
      p.advance();
      while (identifier_char(p.peek()))
        p.advance();
    } else if (p.i + 1 < p.len() && p.peek() == '-'
               && (('a' <= p.peek(1) && p.peek(1) <= 'z') || ('A' <= p.peek(1) && p.peek(1) <= 'Z'))) {
      p.advance(2);
    } else if ('0' <= p.peek() && p.peek() <= '9') {
      p.advance();
      while ('0' <= p.peek() && p.peek() <= '9')
        p.advance();
    } else {
      switch (p.peek()) {
      case '~':
      case '&':
      case '`':
      case '\'':
      case '+':
      case '=':
      case '/':
      case '\\':
      case ',':
      case ';':
      case '.':
      case '_':
      case '*':
      case '?':
      case '!':
      case '@':
      case '<':
      case '>':
      case '$':
        p.advance();
        break;
      default:
        return;
      }
    }
    tokens->push_back({start, p.i, Token::VariableGlobal});
    return;
  }
  case '?': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint32_t start = p.i;
    p.advance();
    if (p.peek() == '\\') {
    combination:
      p.advance();
      if (p.peek() == 'x') {
        p.advance();
        if (is_hex(p.peek()))
          p.advance();
        if (is_hex(p.peek()))
          p.advance();
        tokens->push_back({start, p.i, Token::Char});
        return;
      } else if (p.peek() == 'u') {
        p.advance();
        if (p.peek() == '{') {
          p.advance();
          while (p.peek() != '}' && p.peek() != '\0')
            p.advance();
          if (p.peek() == '}')
            p.advance();
        } else {
          if (is_hex(p.peek()))
            p.advance();
          if (is_hex(p.peek()))
            p.advance();
          if (is_hex(p.peek()))
            p.advance();
          if (is_hex(p.peek()))
            p.advance();
        }
        tokens->push_back({start, p.i, Token::Char});
        return;
      } else if ('0' <= p.peek() && p.peek() <= '7') {
        p.advance();
        if ('0' <= p.peek() && p.peek() <= '7')
          p.advance();
        if ('0' <= p.peek() && p.peek() <= '7')
          p.advance();
        tokens->push_back({start, p.i, Token::Char});
        return;
      } else if (p.peek() == 'c') {
        p.advance();
        if (p.peek() != '\\')
          p.advance();
        else
          goto combination;
        tokens->push_back({start, p.i, Token::Char});
        return;
      } else if (p.peek() == 'M' || p.peek() == 'C') {
        p.advance();
        if (p.peek() == '-') {
          p.advance();
          if (p.peek() != '\\')
            p.advance();
          else
            goto combination;
        }
        tokens->push_back({start, p.i, Token::Char});
        return;
      } else if (p.peek() == 'N') {
        p.advance();
        if (p.peek() == '{') {
          p.advance();
          while (p.peek() != '}' && p.peek() != '\0')
            p.advance();
          if (p.peek() == '}')
            p.advance();
        }
        tokens->push_back({start, p.i, Token::Char});
        return;
      } else {
        p.advance();
        tokens->push_back({start, p.i, Token::Char});
        return;
      }
    } else if (p.peek() != '\0' && p.peek() != ' ' && p.peek() != '\t') {
      p.advance();
      tokens->push_back({start, p.i, Token::Char});
      return;
    } else {
      p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      tokens->push_back({start, p.i, Token::Operator});
      return;
    }
  }
  case '{': {
    p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint8_t brace_color =
      (uint8_t)Token::Brace1 + (p.current().brace_level % 5);
    tokens->push_back({p.i, p.i + 1, (Token::Kind)brace_color});
    p.current().brace_level++;
    p.advance();
    return;
  }
  case '}': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    if (!--p.current().brace_level && p.state->top > 1) {
      p.pop_state();
      tokens->push_back({p.i, p.i + 1, Token::Interpolation});
    } else {
      uint8_t brace_color =
        (uint8_t)Token::Brace1 + (p.current().brace_level % 5);
      tokens->push_back({p.i, p.i + 1, (Token::Kind)brace_color});
    }
    p.advance();
    return;
  }
  case '(': {
    p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint8_t brace_color =
      (uint8_t)Token::Brace1 + (p.current().brace_level % 5);
    tokens->push_back({p.i, p.i + 1, (Token::Kind)brace_color});
    p.current().brace_level++;
    p.advance();
    return;
  }
  case ')': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    p.current().brace_level--;
    uint8_t brace_color =
      (uint8_t)Token::Brace1 + (p.current().brace_level % 5);
    tokens->push_back({p.i, p.i + 1, (Token::Kind)brace_color});
    p.advance();
    return;
  }
  case '[': {
    p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    uint8_t brace_color =
      (uint8_t)Token::Brace1 + (p.current().brace_level % 5);
    tokens->push_back({p.i, p.i + 1, (Token::Kind)brace_color});
    p.current().brace_level++;
    p.advance();
    return;
  }
  case ']': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    p.current().brace_level--;
    uint8_t brace_color =
      (uint8_t)Token::Brace1 + (p.current().brace_level % 5);
    tokens->push_back({p.i, p.i + 1, (Token::Kind)brace_color});
    p.advance();
    return;
  }
  case '\'': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    tokens->push_back({p.i, p.i + 1, Token::String});
    p.current().state = RubyState::RubyInternalState::STRING;
    p.current().delim_start = '\'';
    p.current().delim_end = '\'';
    p.current().flags &= ~RubyState::RubyInternalState::ALLOW_INTERPOLATION;
    p.advance();
    return;
  }
  case '"': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    tokens->push_back({p.i, p.i + 1, Token::String});
    p.current().state = RubyState::RubyInternalState::STRING;
    p.current().delim_start = '"';
    p.current().delim_end = '"';
    p.current().flags |= RubyState::RubyInternalState::ALLOW_INTERPOLATION;
    p.advance();
    return;
  }
  case '`': {
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    tokens->push_back({p.i, p.i + 1, Token::String});
    p.current().state = RubyState::RubyInternalState::STRING;
    p.current().delim_start = '`';
    p.current().delim_end = '`';
    p.current().flags |= RubyState::RubyInternalState::ALLOW_INTERPOLATION;
    p.advance();
    return;
  }
  case '%': {
    if (p.i + 1 >= p.len()) {
      tokens->push_back({p.i, p.i + 1, Token::Operator});
      p.advance();
      p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      return;
    }
    p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    char type = p.peek(1);
    char delim_start = '\0';
    char delim_end = '\0';
    bool allow_interp = true;
    int prefix_len = 1;
    bool is_regexp = false;
    switch (type) {
    case 'r':
      is_regexp = true;
      allow_interp = true;
      prefix_len = 2;
      break;
    case 'Q':
    case 'x':
    case 'I':
    case 'W':
      allow_interp = true;
      prefix_len = 2;
      break;
    case 'w':
    case 'q':
    case 'i':
    case 's':
      allow_interp = false;
      prefix_len = 2;
      break;
    default:
      allow_interp = true;
      prefix_len = 1;
      break;
    }
    if (p.i + prefix_len >= p.len()) {
      tokens->push_back({p.i, p.i + 1, Token::Operator});
      p.advance(prefix_len);
      p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      return;
    }
    delim_start = p.peek(prefix_len);
    if (identifier_char(delim_start) || delim_start == ' ') {
      tokens->push_back({p.i, p.i + 1, Token::Operator});
      p.advance(prefix_len);
      p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      return;
    }
    switch (delim_start) {
    case '(':
      delim_end = ')';
      break;
    case '{':
      delim_end = '}';
      break;
    case '[':
      delim_end = ']';
      break;
    case '<':
      delim_end = '>';
      break;
    default:
      delim_end = delim_start;
      break;
    }
    tokens->push_back({p.i, p.i + prefix_len + 1, (is_regexp ? Token::Regexp : Token::String)});
    p.current().state = is_regexp ? RubyState::RubyInternalState::REGEXP : RubyState::RubyInternalState::STRING;
    p.current().delim_start = delim_start;
    p.current().delim_end = delim_end;
    if (allow_interp)
      p.current().flags |= RubyState::RubyInternalState::ALLOW_INTERPOLATION;
    p.current().lit_brace_level = 1;
    p.advance(prefix_len + 1);
    return;
  }
  case ';':
    p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
    p.advance();
    return;
  default:
    if ('0' <= p.peek() && p.peek() <= '9') {
      p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      uint32_t start = p.i;
      if (p.peek() == '0'
          && (p.peek(1) == 'X' || p.peek(1) == 'x' || p.peek(1) == 'b' || p.peek(1) == 'B' || p.peek(1) == 'o' || p.peek(1) == 'O')) {
        p.advance();
        if (p.peek() == 'x' || p.peek() == 'X') {
          p.advance();
          while (true) {
            while (is_hex(p.peek()))
              p.advance();
            if (p.peek() == '_')
              p.advance();
            else
              break;
          }
        } else if (p.peek() == 'b' || p.peek() == 'B') {
          p.advance();
          while (true) {
            while (p.peek() == '0' || p.peek() == '1')
              p.advance();
            if (p.peek() == '_')
              p.advance();
            else
              break;
          }
        } else if (p.peek() == 'o' || p.peek() == 'O') {
          p.advance();
          while (true) {
            while (p.peek() >= '0' && p.peek() <= '7')
              p.advance();
            if (p.peek() == '_')
              p.advance();
            else
              break;
          }
        }
      } else {
        while (true) {
          while (p.peek() >= '0' && p.peek() <= '9')
            p.advance();
          if (p.peek() == '_')
            p.advance();
          else
            break;
        }
        if (p.peek() == '.') {
          p.advance();
          while (true) {
            while (p.peek() >= '0' && p.peek() <= '9')
              p.advance();
            if (p.peek() == '_')
              p.advance();
            else
              break;
          }
        }
        if (p.peek() == 'E' || p.peek() == 'e') {
          p.advance();
          if (p.peek() == '+' || p.peek() == '-')
            p.advance();
          while (true) {
            while (p.peek() >= '0' && p.peek() <= '9')
              p.advance();
            if (p.peek() == '_')
              p.advance();
            else
              break;
          }
        }
      }
      tokens->push_back({start, p.i, Token::Number});
      return;
    } else if (identifier_start_char(p.peek())) {
      uint32_t j = 1;
      while (identifier_char(p.peek(j)))
        j++;
      if (p.peek(j) == '!' || p.peek(j) == '?')
        j++;
      if (j == tries.base_keywords_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Keyword});
        p.advance(j);
        return;
      } else if (j == tries.expecting_keywords_trie.longest_match(p.peek_str(j))) {
        p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Keyword});
        p.advance(j);
        return;
      } else if (j == tries.operator_keywords_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::KeywordOperator});
        p.advance(j);
        return;
      } else if (j == tries.expecting_operators_trie.longest_match(p.peek_str(j))) {
        p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::KeywordOperator});
        p.advance(j);
        return;
      } else if (j == tries.types_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Type});
        p.advance(j);
        return;
      } else if (j == tries.methods_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Function});
        p.advance(j);
        return;
      } else if (j == tries.expecting_end_keywords_trie.longest_match(p.peek_str(j))) {
        events->push_back({p.peek_str(j), ParseEvent::Opening, 0});
        p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Keyword});
        p.advance(j);
        return;
      } else if (j == tries.conditional_keywords_trie.longest_match(p.peek_str(j))) {
        if (p.current().flags & RubyState::RubyInternalState::EXPECTING_EXPRESSION)
          events->push_back({p.peek_str(j), ParseEvent::Opening, 0});
        p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Keyword});
        p.advance(j);
        return;
      } else if (j == tries.end_keywords_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        ScopeTypes kind = p.peek() == 'd' ? ScopeTypes::Block : ScopeTypes::None;
        events->push_back({p.peek_str(j), ParseEvent::Opening, (uint8_t)kind});
        tokens->push_back({p.i, p.i + j, Token::Keyword});
        p.advance(j);
        return;
      } else if (j == tries.builtins_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Constant});
        p.advance(j);
        return;
      } else if (j == tries.errors_trie.longest_match(p.peek_str(j))) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        tokens->push_back({p.i, p.i + j, Token::Error});
        p.advance(j);
        return;
      } else if ('A' <= p.peek() && p.peek() <= 'Z' && !(p.peek(j) == '!' || p.peek(j) == '?')) {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        if (j >= 5 && p.peek_str(j).substr(j - 5) == "Error") {
          tokens->push_back({p.i, p.i + j, Token::Error});
          p.advance(j);
          return;
        }
        tokens->push_back({p.i, p.i + j, Token::Constant});
        p.advance(j);
        return;
      } else {
        p.current().flags &= ~RubyState::RubyInternalState::EXPECTING_EXPRESSION;
        if (j == 4 && p.peek_str(4) == "true") {
          tokens->push_back({p.i, p.i + j, Token::True});
          p.advance(4);
          return;
        }
        if (j == 5 && p.peek_str(5) == "false") {
          tokens->push_back({p.i, p.i + j, Token::False});
          p.advance(5);
          return;
        }
        if (j == 3 && p.peek_str(3) == "end") {
          events->push_back({p.peek_str(3), ParseEvent::Closing, 0});
          tokens->push_back({p.i, p.i + j, Token::Keyword});
          p.advance(3);
          return;
        }
        if (j == 5 && p.peek_str(5) == "class") {
          tokens->push_back({p.i, p.i + j, Token::Keyword});
          p.advance(5);
          p.current().flags =
            (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK)
            | RubyState::RubyInternalState::CLASS_NAME;
          return;
        }
        if (j == 6 && p.peek_str(6) == "module") {
          tokens->push_back({p.i, p.i + j, Token::Keyword});
          p.advance(6);
          p.current().flags =
            (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK)
            | RubyState::RubyInternalState::MODULE_NAME;
          return;
        }
        if (j == 3 && p.peek_str(3) == "def") {
          tokens->push_back({p.i, p.i + j, Token::Keyword});
          p.advance(3);
          p.current().flags =
            (p.current().flags & ~RubyState::RubyInternalState::NAME_MASK)
            | RubyState::RubyInternalState::DEF_NAME;
          return;
        }
        uint32_t start = p.i;
        p.advance(j);
        if (p.peek() == ':') {
          p.advance();
          tokens->push_back({start, p.i, Token::Label});
          return;
        } else if (p.peek() == '!' || p.peek() == '?') {
          p.advance();
          tokens->push_back({start, p.i, Token::Function});
          return;
        } else {
          uint32_t j = 0;
          if (p.peek(j) == '(' || p.peek(j) == '{') {
            tokens->push_back({start, p.i, Token::Function});
            return;
          } else if (p.peek(j) == ' ' || p.peek(j) == '\t') {
            j++;
          } else {
            return;
          }
          while (p.peek(j) == ' ' || p.peek(j) == '\t')
            j++;
          if (p.i + j >= p.len())
            return;
          if (
            p.peek(j) == '-'
            || p.peek(j) == '&'
            || p.peek(j) == '%'
            || p.peek(j) == ':'
          ) {
            if (p.peek(j + 1) == ' ' || p.peek(j + 1) == '>')
              return;
          } else if (
            p.peek(j) == ']'
            || p.peek(j) == '}'
            || p.peek(j) == ')'
            || p.peek(j) == ','
            || p.peek(j) == ';'
            || p.peek(j) == '.'
            || p.peek(j) == '+'
            || p.peek(j) == '*'
            || p.peek(j) == '/'
            || p.peek(j) == '='
            || p.peek(j) == '?'
            || p.peek(j) == '|'
            || p.peek(j) == '^'
            || p.peek(j) == '<'
            || p.peek(j) == '>'
          ) {
            return;
          }
          tokens->push_back({start, p.i, Token::Function});
        }
      }
    } else {
      uint32_t op_len;
      if ((op_len = tries.operator_trie.longest_match(p.peek_str(p.len() - p.i)))) {
        tokens->push_back({p.i, p.i + op_len, Token::Operator});
        p.advance(op_len);
        p.current().flags |= RubyState::RubyInternalState::EXPECTING_EXPRESSION;
      } else {
        p.advance(utf8_codepoint_width(p.peek()));
      }
    }
  }
}

void ruby_parse(
  void **v_state,
  std::string_view line,
  bool fl,
  std::vector<Token> *tokens,
  std::vector<ParseEvent> *events
) {
  RubyParser p(v_state, line);
  while (p.i < p.len()) {
    if (p.current().state == RubyState::RubyInternalState::END)
      return;
    if (p.current().state == RubyState::RubyInternalState::COMMENT) {
      tokens->push_back({p.i, p.len(), Token::Comment});
      if (p.i == 0 && p.peek_str(4) == "=end") {
        p.current().state = RubyState::RubyInternalState::NONE;
        events->push_back({{nullptr, 0}, ParseEvent::Closing, 0});
      }
      return;
    }
    if (!p.heredoc_start_line
        && p.current().state == RubyState::RubyInternalState::HEREDOC) {
      if (handle_heredoc(p, tokens))
        return;
      else
        continue;
    }
    if (p.current().state == RubyState::RubyInternalState::STRING) {
      handle_string(p, tokens);
      continue;
    }
    if (p.current().state == RubyState::RubyInternalState::REGEXP) {
      handle_regex(p, tokens);
      continue;
    }
    if (!p.i && handle_line_markers(p, tokens, events))
      return;
    if (handle_comment(p, tokens, fl))
      return;
    handle_syntax(p, tokens, events);
    continue;
  }
}
} // namespace bed::internal::syntax::ruby
