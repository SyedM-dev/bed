#pragma once

#include "pch.h"

namespace bed::internal::trie {
template <typename T = void>
struct Trie {
  using V = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

  struct Node {
    std::string edge;
    std::optional<V> value{};
    std::vector<Node *> children;
    Node(std::string e = {}) : edge(std::move(e)) {}
    ~Node() {
      for (auto *c : children)
        delete c;
    };
  } root;

  bool case_sensitive;

  Trie(bool cs = true) : case_sensitive(cs) {}

  void insert(std::string_view key)
    requires std::is_void_v<T>
  {
    _insert(key, std::monostate{});
  }

  void insert(std::string_view key, V el)
    requires(!std::is_void_v<T>)
  {
    _insert(key, std::move(el));
  }

  void _insert(std::string_view key, V &&el) {
    Node *current = &root;
    uint64_t pos = 0;
    while (pos < key.size()) {
      Node *child = find_child(*current, key[pos]);
      if (!child) {
        auto *node = new Node(std::string(key.substr(pos)));
        node->value = std::move(el);
        current->children.push_back(node);
        return;
      }
      const auto common = common_prefix(child->edge, key.substr(pos));
      if (common == child->edge.size()) {
        pos += common;
        current = child;
        continue;
      }
      auto *split = new Node(child->edge.substr(0, common));
      child->edge.erase(0, common);
      split->children.push_back(child);
      current->children.erase(
        std::find(current->children.begin(), current->children.end(), child)
      );
      current->children.push_back(split);
      pos += common;
      if (pos == key.size()) {
        split->value = std::move(el);
        return;
      }
      auto *node = new Node(std::string(key.substr(pos)));
      node->value = std::move(el);
      split->children.push_back(node);
      return;
    }
    current->value = std::move(el);
  }

  void remove(std::string_view key) {
    _remove(root, key, 0);
  }

  bool _remove(Node &node, std::string_view key, uint64_t pos) {
    if (pos == key.size()) {
      if (!node.value)
        return false;
      node.value.reset();
      return true;
    }
    Node *child = find_child(node, key[pos]);
    if (!child)
      return false;
    const auto remaining = key.substr(pos);
    const auto common = common_prefix(child->edge, remaining);
    if (common != child->edge.size())
      return false;
    const auto child_pos = pos + common;
    if (!_remove(*child, key, child_pos))
      return false;
    if (!child->value && child->children.empty()) {
      auto it = std::find(
        node.children.begin(),
        node.children.end(),
        child
      );
      node.children.erase(it);
      delete child;
      return true;
    }
    if (!child->value && child->children.size() == 1) {
      Node *grandchild = child->children.front();
      child->edge += grandchild->edge;
      child->value = std::move(grandchild->value);
      child->children = std::move(grandchild->children);
      grandchild->children.clear();
      delete grandchild;
    }
    return true;
  }

  std::vector<std::string> search(std::string_view prefix) {
    std::vector<std::string> result;
    Node *current = &root;
    std::string key;
    uint64_t pos = 0;
    while (pos < prefix.size()) {
      Node *child = find_child(*current, prefix[pos]);
      if (!child)
        return result;
      const auto remaining = prefix.substr(pos);
      const auto common = common_prefix(child->edge, remaining);
      if (common == 0)
        return result;
      if (common < child->edge.size()) {
        if (common == remaining.size()) {
          key += child->edge;
          collect(*child, key, result);
          return result;
        }
        return result;
      }
      key += child->edge;
      pos += common;
      current = child;
    }
    collect(*current, key, result);
    return result;
  }

  static void collect(
    const Node &node,
    std::string &key,
    std::vector<std::string> &result
  ) {
    if (node.value)
      result.push_back(key);
    for (auto *child : node.children) {
      const auto old_size = key.size();
      key += child->edge;
      collect(*child, key, result);
      key.resize(old_size);
    }
  }

  uint64_t longest_match(std::string_view input) {
    Node *current = &root;
    uint64_t pos = 0;
    uint64_t longest = 0;
    if (current->value)
      longest = 0;
    while (pos < input.size()) {
      Node *child = find_child(*current, input[pos]);
      if (!child)
        break;
      const auto remaining = input.substr(pos);
      const auto common = common_prefix(child->edge, remaining);
      if (common != child->edge.size())
        break;
      pos += common;
      current = child;
      if (current->value)
        longest = pos;
    }
    return longest;
  }

  bool matches(std::string_view key)
    requires(std::is_void_v<T>)
  {
    Node *current = &root;
    uint64_t pos = 0;
    while (pos < key.size()) {
      Node *child = find_child(*current, key[pos]);
      if (!child)
        return false;
      const auto remaining = key.substr(pos);
      const auto common = common_prefix(child->edge, remaining);
      if (common != child->edge.size())
        return false;
      pos += common;
      current = child;
    }
    return current->value.has_value();
  }

  std::optional<V> get(std::string_view key)
    requires(!std::is_void_v<T>)
  {
    Node *current = &root;
    uint64_t pos = 0;
    while (pos < key.size()) {
      Node *child = find_child(*current, key[pos]);
      if (!child)
        return std::nullopt;
      const auto remaining = key.substr(pos);
      const auto common = common_prefix(child->edge, remaining);
      if (common != child->edge.size())
        return std::nullopt;
      pos += common;
      current = child;
    }
    if (!current->value)
      return std::nullopt;
    return *current->value;
  }

  bool equal_char(char a, char b) const {
    if (case_sensitive)
      return a == b;
    return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
  }

  uint64_t common_prefix(
    std::string_view a,
    std::string_view b
  ) {
    const auto n = std::min(a.size(), b.size());
    uint64_t i = 0;
    while (i < n && equal_char(a[i], b[i]))
      ++i;
    return i;
  }

  Node *find_child(Node &node, char first) {
    for (auto *child : node.children)
      if (equal_char(child->edge.front(), first))
        return child;
    return nullptr;
  }
};
} // namespace bed::internal::trie
