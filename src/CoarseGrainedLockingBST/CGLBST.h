#pragma once

#include <shared_mutex>

#include "CGLBSTNode.h"

template <class T>
struct CGLBST {
  CGLBSTNode<T>* root = nullptr;
  std::shared_mutex mut{};

  bool operator[](const T& key) {
    std::shared_lock<std::shared_mutex> lk{mut};
    CGLBSTNode<T>* curNode = root;

    while (curNode != nullptr) {
      if (key == curNode->key)
        return true;
      else if (key < curNode->key)
        curNode = curNode->left;
      else
        curNode = curNode->right;
    }
    return false;
  }

  bool insert(const T& key) {
    std::unique_lock<std::shared_mutex> lk{mut};
    if (root == nullptr) {
      root = new CGLBSTNode<T>(key);
      return true;
    }

    CGLBSTNode<T>* cur = root;

    while (cur->key != key) {
      if (key < cur->key) {
        if (cur->left == nullptr) {
          cur->left = new CGLBSTNode<T>(key);
          return true;
        }
        cur = cur->left;
      } else {
        if (cur->right == nullptr) {
          cur->right = new CGLBSTNode<T>(key);
          return true;
        }
        cur = cur->right;
      }
    }

    return false;
  }

  bool remove(const T& key) {
    std::unique_lock<std::shared_mutex> lk{mut};
    if (root == nullptr)
      return false;

    CGLBSTNode<T>**curPtr = &root, *cur = root;

    while (cur->key != key) {
      if (key < cur->key) {
        if (cur->left == nullptr)
          return false;
        curPtr = &cur->left;
        cur = cur->left;
      } else {
        if (cur->right == nullptr)
          return false;
        curPtr = &cur->right;
        cur = cur->right;
      }
    }

    if (cur->left == nullptr) {
      *curPtr = cur->right;
      return true;
    } else if (cur->right == nullptr) {
      *curPtr = cur->left;
      return true;
    }

    CGLBSTNode<T>** inorderSuccessorPtr = &(cur->left);
    CGLBSTNode<T>* inorderSuccessor = cur->left;
    while (inorderSuccessor->right != nullptr) {
      inorderSuccessorPtr = &(inorderSuccessor->right);
      inorderSuccessor = inorderSuccessor->right;
    }

    cur->key = inorderSuccessor->key;
    *inorderSuccessorPtr = inorderSuccessor->left;
    return true;
  }
};