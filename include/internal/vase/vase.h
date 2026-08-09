#pragma once

#include "buffer/append.h"
#include "buffer/original.h"
#include "constants.h"
#include "iterators/line.h"
#include "pch.h"
#include "shard.h"

namespace crib::internal::vase {
struct Point {
  uint64_t row;
  uint64_t col;
};

struct Range {
  Point start;
  Point end;
};

struct Vase {
  struct RegexGroup {
    uint64_t start{0};
    uint64_t end{0};
  };

  struct RegexMatch {
    uint64_t start;
    uint64_t end;
    RegexGroup groups[9]{};
  };

  struct ReplacePart {
    enum struct PartType {
      FullMatch,
      CaptureGroup,
      Constant
    } type;

    std::variant<uint8_t, Shard *> value;
  };

  OriginalBuffer *original;
  AppendBuffer *append;
  Shard *root;

#ifdef _WIN32
  bool posix_ending = false;
  bool using_crlf = true;
#else
  bool posix_ending = true;
  bool using_crlf = false;
#endif

  std::filesystem::path path;
  std::filesystem::path swapdir;

  Vase(std::filesystem::path path, std::filesystem::path swapdir);
  Vase(std::string cmd, std::filesystem::path swapdir);
  Vase(std::filesystem::path swapdir);
  ~Vase();
  Vase(Vase &&other) noexcept;
  Vase &operator=(Vase &&other) noexcept;
  Vase(const Vase &) = delete;
  Vase &operator=(const Vase &) = delete;

  uint64_t length();
  uint64_t lines();
  std::string to_string();
  std::string to_string(Range range);

  Iterator iterate(uint64_t line, Direction dir);

  void insert(Point *point, char key);
  void insert(Point *point, std::string_view str);
  void insert(Point *point, const char *data, uint64_t len);
  void erase(Point *point, uint64_t amount, Direction dir);
  void erase(Range range);
  void replace(Range range, std::string_view str);
  void replace(Range range, const char *data, uint64_t len);

  void move_clusters(Point *point, uint64_t amount, Direction dir);
  void move_lines(Point *point, uint64_t amount, Direction dir);
  void clamp(Point *point);

  void regex_search_replace(
    std::string_view pattern, Range range,
    std::string_view replace, std::string_view options
  );

  std::vector<Range> regex_search(
    std::string_view pattern, Range range, std::string_view options
  );

  bool undo();
  bool redo();
  void snapshot();
  void prune_history(uint64_t n);

  bool save();
  bool save_swap();

  uint64_t offset_of(Point point);
  Point point_of(uint64_t offset);

private:
  std::vector<Shard *> history;
  uint64_t history_top;

  void _insert(Point *point, const char *data, uint64_t len);

  std::vector<ReplacePart> parse_replace(std::string_view s);
  std::vector<RegexMatch> _regex_search(
    std::string_view pattern, Range range, std::string_view options
  );
};
} // namespace crib::internal::vase
