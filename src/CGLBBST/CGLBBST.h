#pragma once

#include <mutex>
#include <set>
#include <shared_mutex>

template <typename T>
struct CGLBBST {
  std::set<T> tree;
  std::shared_mutex mut{};

  bool operator[](const T& key) {
    std::shared_lock lk{mut};
    return tree.contains(key);
  }

  bool insert(const T& key) {
    std::unique_lock lk{mut};
    if (tree.contains(key))
      return false;
    tree.insert(key);
    return true;
  }

  bool remove(const T& key) {
    std::unique_lock lk{mut};
    if (!tree.contains(key))
      return false;
    tree.erase(key);
    return true;
  }
};