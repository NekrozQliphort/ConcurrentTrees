#pragma once

#include <algorithm>

#include "Operation.h"

template <class T>
struct Node {
  T key;
  std::atomic<Node<T>*> left{}, right{};
  std::atomic<OperationFlaggedPointer>
      op{};  // require uintptr_t as we need last 2 bits for flagging
  int local_height{}, lh{}, rh{};
  std::atomic<bool> deleted{}, removed{};

  explicit Node(T key, Node<T>* left = nullptr, Node<T>* right = nullptr)
      : key{}, left{left}, right{right} {}
};

template <typename T>
Operation<T>* getPointer(OperationFlaggedPointer ptr) {
  return reinterpret_cast<Operation<T>*>(ptr &
                                         OperationConstants::POINTER_MASK);
}

template <typename T>
Operation<T>* unFlag(std::atomic<OperationFlaggedPointer>& op) {
  return reinterpret_cast<Operation<T>*>(op &=
                                         OperationConstants::POINTER_MASK);
}

template <typename T>
OperationFlaggedPointer flag(Operation<T>* op,
                             OperationConstants::Flags flags) {
  return reinterpret_cast<OperationFlaggedPointer>(op) | flags;
}