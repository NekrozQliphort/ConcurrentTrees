#pragma once
#include <variant>

// Forward Declaration
namespace OperationConstants {
enum Flags : uintptr_t { NONE, MARK, ROTATE, INSERT };

constexpr uintptr_t FLAG_MASK = 3;
constexpr uintptr_t POINTER_MASK = ~FLAG_MASK;
}  // namespace OperationConstants

template <typename T>
struct InsertOp;

template <typename T>
struct RotateOp;

template <typename T>
struct Node;

template <typename T>
using Operation = std::variant<InsertOp<T>, RotateOp<T>>;

using OperationFlaggedPointer = uintptr_t;

template <typename T>
struct InsertOp {
  bool isLeft;
  bool isUpdate{false};
  Node<T>* expectedNode;
  Node<T>* newNode;
};

template <typename T>
struct RotateOp {
  int state = -1;
  uintptr_t parent, node, child;
  Operation<T>*pOp, *nOp, *cOp;
  bool rightR, dir;
};

OperationConstants::Flags getFlag(OperationFlaggedPointer ptr) {
  return static_cast<OperationConstants::Flags>(ptr &
                                                OperationConstants::FLAG_MASK);
}