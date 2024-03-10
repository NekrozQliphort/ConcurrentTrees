#pragma once

#include "Node.h"
#include "Operation.h"

enum class SeekResultState { NOT_FOUND_L, NOT_FOUND_R, FOUND };

template <typename T>
struct SeekRecord {
  SeekResultState result;
  Node<T>*parent, *node;
  OperationFlaggedPointer parentOp, nodeOp;
};