#pragma once

#include <algorithm>

#include "Operation.h"

namespace Singh {
template <class T>
struct Node {
  T key;
  std::atomic<Node<T>*> left{}, right{};
  std::atomic<OperationFlaggedPointer>
      op{};  // require uintptr_t as we need last 2 bits for flagging
  int local_height{}, lh{}, rh{};
  std::atomic<uint8_t> deleted{};
  std::atomic<bool> removed{};

  explicit Node(T key, Node<T>* left = nullptr, Node<T>* right = nullptr,
                int local_height = 0, int lh = 0, int rh = 0,
                uint8_t deleted = 0, bool removed = false)
      : key{key},
        left{left},
        right{right},
        local_height{local_height},
        lh{lh},
        rh{rh},
        deleted{deleted},
        removed{removed} {}
};

template <typename T>
Operation<T>* getPointer(OperationFlaggedPointer ptr) {
  return reinterpret_cast<Operation<T>*>(ptr &
                                         OperationConstants::POINTER_MASK);
}

template <typename T>
Operation<T>* unFlag(OperationFlaggedPointer op) {
  return reinterpret_cast<Operation<T>*>(op & OperationConstants::POINTER_MASK);
}

template <typename T>
OperationFlaggedPointer flag(Operation<T>* op,
                             OperationConstants::Flags flags) {
  return reinterpret_cast<OperationFlaggedPointer>(op) | flags;
}
}  // namespace Singh
