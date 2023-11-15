#pragma once


template <class T>
struct CGLBSTNode {
  T key;
  CGLBSTNode<T>*left, *right;

  explicit CGLBSTNode(const T& key, CGLBSTNode* left = nullptr,
                      CGLBSTNode* right = nullptr)
      : key(key), left(left), right(right) {}
};