#pragma once

#include <algorithm>

template <class T, std::size_t ALIGN = std::max(
                       {alignof(T), std::size_t(1 << 2), alignof(void*)})>
struct alignas(ALIGN) Node {
  constexpr static uintptr_t FLAG_MASK = 2;
  constexpr static uintptr_t TAG_MASK = 1;
  constexpr static uintptr_t POINTER_MASK = ~(UINTPTR_MAX & FLAG_MASK);
  static_assert(ALIGN <= alignof(max_align_t), "Over-Aligned");

  T key;
  std::atomic<uintptr_t> left, right;
  explicit Node(const T& key, Node<T>* l = nullptr, Node<T>* r = nullptr)
      : key(key),
        left(std::atomic<uintptr_t>(reinterpret_cast<uintptr_t>(l))),
        right(std::atomic<uintptr_t>(reinterpret_cast<uintptr_t>(r))) {}
};

template <class T>
Node<T>* getPointer(uintptr_t ptr) {
  return reinterpret_cast<Node<T>*>(ptr & Node<T>::POINTER_MASK);
}

template <class T>
uintptr_t getPointerUintRepr(Node<T>* ptr) {
  return reinterpret_cast<uintptr_t>(ptr) & Node<T>::POINTER_MASK;
}

template <class T>
uintptr_t getFlags(uintptr_t ptr) {
  return ptr & (Node<T>::FLAG_MASK | Node<T>::TAG_MASK);
}
