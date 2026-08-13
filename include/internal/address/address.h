#pragma once

#include "definitions.h"
#include "internal/generic.h"
#include "pch.h"

namespace bed::internal::address {
struct address_error : ed_error {
  address_error(const char *msg) : ed_error(msg) {}
};

struct Address {
  // % refers to range of whatever was resulted from the previous modification.
  // it expands in theory to a Num,Num and so can be followed by chaining more adresses the ed way.
  // this is handled and resolved by the handle function,
  // the constuctor and resolve etc. are also only called in the handle function,
  // i.e. handle is the only public facing API from this namespace & class for now.
  //
  // in case of any issue an instance of address_error(const char *) is thrown.

  struct Result {
    std::array<uint64_t, 2> data{};
    uint8_t size{0};
    std::span<const uint64_t> span(uint8_t max = UINT8_MAX) const {
      return {data.data(), std::min(size, max)};
    }
  };

  struct None {};

  // Posix ed types of addressing.
  struct Current {}; // .

  struct Last {}; // $

  struct Number { // n
    uint64_t i;
  };

  struct Mark { // 'm
    char m;
  };

  struct RegexLine { // /re/ or ?re?
    internal::Direction dir;
    std::string re;
  };

  // BEd Extended types of addressing.
  struct Regex {             // &/re/ or &?re?
    internal::Direction dir; // returns next/previous line containing the regex
    std::string re;          // (not only if it matches full line)
  };

  struct Diagnostic { // #n# for diagnostic number n. (From lsp.)
    uint16_t n;
  };

  struct DiagnosticNext { // ^ or % for previous/next diagnostic
    internal::Direction dir;
  };

  struct SymbolDefinition { // <sym> goto symbol definition. (From lsp or using internal language parsers)
    std::string sym;
  };

  struct SymbolReference { // <sym:n> goto nth symbol reference
    uint16_t n;
    std::string sym;
  };

  struct SymbolReferenceNext { // >sym> or <sym<
    internal::Direction dir;   // goto next or previous symbol reference
    std::string sym;
  };

  struct Block { // [ or ] goto start/end of containing block.
    internal::Direction dir;
  };

  struct Scripted {   // (function_name:arguments)
    std::string func; // Calls ruby mapping with name giving the argument as string.
    std::string arg;  // resolves to line number returned by function (or throws error).
                      // The mruby runtime also has the full context of the file and extentions etc.
  };

  std::variant<
    std::monostate,
    None,
    Current,
    Last,
    Number,
    Mark,
    RegexLine,
    Regex,
    Diagnostic,
    DiagnosticNext,
    SymbolDefinition,
    SymbolReference,
    SymbolReferenceNext,
    Block,
    Scripted>
    base{};

  // they can then be followed by any number of +n or -n etc accumulating in.
  int64_t offset = 0;

  Address(std::string &cmd, uint64_t &i);
  Address() = default;

  uint64_t resolve(BEd &ctx);

  static Result handle(BEd &ctx, std::string &cmd, uint64_t &i);
};
} // namespace bed::internal::address
