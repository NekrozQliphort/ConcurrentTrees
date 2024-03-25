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
  InsertOp(bool isLeft, Node<T>* expectedNode, Node<T>* newNode)
      : InsertOp{isLeft, false, expectedNode, newNode} {}
  InsertOp(bool isLeft, bool isUpdate, Node<T>* expectedNode, Node<T>* newNode)
      : isLeft{isLeft},
        isUpdate{isUpdate},
        expectedNode{expectedNode},
        newNode{newNode} {}
};

template <typename T>
struct RotateOp {
  constexpr static int UNDECIDED = 0, GRABBED_FIRST = 1, GRABBED_SECOND = 2,
                       ROTATED = 3, DONE = 4;

  std::atomic<Node<T>*> grandchild;
  std::atomic<int> state{0};

  Node<T>*parent, *node, *child;
  const bool isLeftRotation;  // is it left rotation
  const bool isLeftChild;     // is node the left child of parent

  RotateOp(Node<T>* parent, Node<T>* node, Node<T>* child, bool isLeftRotation, bool isLeftChild, Node<T>* grandchild)
      : grandchild{grandchild},
        parent{parent},
        node{node},
        child{child},
        isLeftRotation{isLeftRotation},
        isLeftChild{isLeftChild} {}
};

OperationConstants::Flags getFlag(OperationFlaggedPointer ptr) {
  return static_cast<OperationConstants::Flags>(ptr &
                                                OperationConstants::FLAG_MASK);
}