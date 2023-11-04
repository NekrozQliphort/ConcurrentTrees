#pragma once
#include "Node.h"

template <class T>
struct SeekRecord {
  Node<T> *ancestor, *successor, *parent, *leaf;
};
