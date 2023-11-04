#pragma once

#include <shared_mutex>
#include <atomic>

template <class T>
struct FGLBSTNode {
  T key;
  std::shared_mutex mut;
  FGLBSTNode<T> *left, *right;


  explicit FGLBSTNode(const T& key, FGLBSTNode* left = nullptr, FGLBSTNode* right = nullptr):
    key(key), mut(), left(left), right(right) {}
};