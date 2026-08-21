#pragma once

#include "internal/trie/trie.h"
#include "pch.h"

namespace bed::internal::syntax::ruby {
const static std::vector<std::string> types = {
  "BasicObject",
  "Object",
  "NilClass",
  "TrueClass",
  "FalseClass",
  "Integer",
  "Fixnum",
  "Bignum",
  "Float",
  "Rational",
  "Complex",
  "Numeric",
  "String",
  "Symbol",
  "Array",
  "Hash",
  "Range",
  "Regexp",
  "Struct",
  "Enumarator",
  "Enumerable",
  "Time",
  "Date",
  "IO",
  "File",
  "Dir",
  "Thread",
  "Proc",
  "Method",
  "Module",
  "Class",
  "Mutex",
  "ConditionVariable",
  "MatchData",
  "Encoding",
  "Fiber",
};

const static std::vector<std::string> builtins = {
  "ARGF",
  "ARGV",
  "ENV",
  "STDIN",
  "STDOUT",
  "STDERR",
  "DATA",
  "TOPLEVEL_BINDING",
  "RUBY_PLATFORM",
  "RUBY_VERSION",
  "RUBY_RELEASE_DATE",
  "RUBY_PATCHLEVEL",
  "RUBY_ENGINE",
  "__LINE__",
  "__FILE__",
  "__ENCODING__",
  "__dir__",
  "__callee__",
  "__method__",
  "__id__",
  "__send__",
};

const static std::vector<std::string> methods = {
  "abort",
  "at_exit",
  "binding",
  "block_given?",
  "caller",
  "catch",
  "chomp",
  "chomp!",
  "chop",
  "chop!",
  "eval",
  "exec",
  "exit",
  "exit!",
  "fail",
  "fork",
  "format",
  "gets",
  "global_variables",
  "gsub",
  "gsub!",
  "iterator?",
  "lambda",
  "load",
  "loop",
  "open",
  "print",
  "printf",
  "proc",
  "putc",
  "puts",
  "raise",
  "rand",
  "readline",
  "readlines",
  "require",
  "require_relative",
  "select",
  "sleep",
  "spawn",
  "split",
  "sprintf",
  "srand",
  "sub",
  "sub!",
  "syscall",
  "system",
  "test",
  "throw",
  "trace_var",
  "trap",
  "untrace_var",
  "attr",
  "attr_reader",
  "attr_writer",
  "attr_accessor",
  "class_variable_get",
  "class_variable_set",
  "define_method",
  "instance_variable_get",
  "instance_variable_set",
  "private",
  "protected",
  "public",
  "public_class_method",
  "module_function",
  "remove_method",
  "undef_method",
  "method",
  "methods",
  "singleton_methods",
  "private_methods",
  "protected_methods",
  "public_methods",
  "send",
  "extend",
  "include",
  "prepend",
  "clone",
  "dup",
  "freeze",
  "taint",
  "untaint",
  "trust",
  "untrust",
  "untaint?",
  "trust?",
  "each",
  "each_with_index",
  "each_with_object",
  "map",
  "collect",
  "select",
  "reject",
  "reduce",
  "inject",
  "find",
  "detect",
  "all?",
  "any?",
  "none?",
  "one?",
  "count",
  "cycle",
  "drop",
  "drop_while",
  "take",
  "take_while",
  "chunk",
  "chunk_while",
  "group_by",
  "partition",
  "slice_before",
  "slice_after",
  "nil?",
  "is_a?",
  "kind_of?",
  "instance_of?",
  "respond_to?",
  "equal?",
  "object_id",
  "singleton_class",
  "clone",
  "freeze",
  "tap",
  "then",
};

const static std::vector<std::string> errors = {
  "Error",
  "Exception",
  "SignalException",
  "Interrupt",
  "StopIteration",
  "Errno",
  "SystemExit",
  "fatal",
};

const static std::vector<std::string> base_keywords = {
  "else",
  "rescue",
  "ensure",
};

const static std::vector<std::string> expecting_keywords = {
  "when",
  "elsif",
};

const static std::vector<std::string> expecting_end_keywords = {
  "case",
  "for",
};

const static std::vector<std::string> conditional_keywords = {
  "if",
  "unless",
  "while",
  "until",
};

const static std::vector<std::string> end_keywords = {
  "begin",
  "do",
};

const static std::vector<std::string> operator_keywords = {
  "alias",
  "BEGIN",
  "break",
  "catch",
  "defined?",
  "in",
  "next",
  "redo",
  "rescue",
  "retry",
  "self",
  "nil",
  "undef",
};

const static std::vector<std::string> expecting_operators = {
  "super",
  "and",
  "return",
  "not",
  "yield",
  "or",
};

const static std::vector<std::string> operators = {
  "+",
  "-",
  "*",
  "/",
  "%",
  "**",
  "==",
  "!=",
  "===",
  "<=>",
  ">",
  ">=",
  "<",
  "<=",
  "&&",
  "||",
  "!",
  "&",
  "|",
  "^",
  "~",
  "<<",
  ">>",
  "=",
  "+=",
  "-=",
  "*=",
  "/=",
  "%=",
  "**=",
  "&=",
  "|=",
  "^=",
  "<<=",
  ">>=",
  "..",
  "...",
  "===",
  "=",
  "=>",
  "&",
  "`",
  "->",
  "=~",
};

struct RubyTries {
  trie::Trie<void> base_keywords_trie;
  trie::Trie<void> expecting_keywords_trie;
  trie::Trie<void> operator_keywords_trie;
  trie::Trie<void> expecting_operators_trie;
  trie::Trie<void> operator_trie;
  trie::Trie<void> types_trie;
  trie::Trie<void> builtins_trie;
  trie::Trie<void> methods_trie;
  trie::Trie<void> errors_trie;
  trie::Trie<void> expecting_end_keywords_trie;
  trie::Trie<void> conditional_keywords_trie;
  trie::Trie<void> end_keywords_trie;
  RubyTries() {
    for (auto &keyword : base_keywords)
      base_keywords_trie.insert(keyword);
    for (auto &keyword : expecting_keywords)
      expecting_keywords_trie.insert(keyword);
    for (auto &keyword : operator_keywords)
      operator_keywords_trie.insert(keyword);
    for (auto &keyword : expecting_operators)
      expecting_operators_trie.insert(keyword);
    for (auto &keyword : operators)
      operator_trie.insert(keyword);
    for (auto &keyword : types)
      types_trie.insert(keyword);
    for (auto &keyword : builtins)
      builtins_trie.insert(keyword);
    for (auto &keyword : methods)
      methods_trie.insert(keyword);
    for (auto &keyword : errors)
      errors_trie.insert(keyword);
    for (auto &keyword : expecting_end_keywords)
      expecting_end_keywords_trie.insert(keyword);
    for (auto &keyword : conditional_keywords)
      conditional_keywords_trie.insert(keyword);
    for (auto &keyword : end_keywords)
      end_keywords_trie.insert(keyword);
  }
};
} // namespace bed::internal::syntax::ruby
