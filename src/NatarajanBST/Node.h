#pragma once

#include <atomic>
#include <bit>

template <class T>
struct Node {
  using Data = __int128;

  struct NodePtrWithFlags {
    Node<T>* ptr;
    uint32_t flags;

    NodePtrWithFlags() = default;
    explicit NodePtrWithFlags(Node<T>* ptr, uint32_t flags = 0)
        : ptr(ptr), flags(flags) {}
  };

  T key;
  std::atomic<Data> left, right;
  explicit Node(const T& key, Node<T>* l = nullptr, Node<T>* r = nullptr)
      : key(key),
        left(std::atomic<Data>(std::bit_cast<Data>(NodePtrWithFlags(l)))),
        right(std::atomic<Data>(std::bit_cast<Data>(NodePtrWithFlags(r)))) {}
};
