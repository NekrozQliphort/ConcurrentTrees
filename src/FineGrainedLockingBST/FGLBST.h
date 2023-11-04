#pragma once

#include <limits>
#include <utility>

#include "FGLBSTNode.h"

template <class T, T inf0 = std::numeric_limits<T>::max() - 1, T inf1 = std::numeric_limits<T>::max()>
struct FGLBST {
  FGLBSTNode<T>* root = new FGLBSTNode<T>(inf1, new FGLBSTNode<T>(inf0));

  bool operator[](const T& key) {
    std::shared_lock<std::shared_mutex> lk{root->mut};
    FGLBSTNode<T>* curNode = root;
    // insert

    while (true) {
      if (key == curNode->key) return true;
      else if (key < curNode->key) {
        if (curNode->left == nullptr) return false;
        curNode = curNode->left;
        std::shared_lock<std::shared_mutex> childLk{curNode->mut};
        lk = std::move(childLk);
      } else {
        if (curNode->right == nullptr) return false;
        curNode = curNode->right;
        std::shared_lock<std::shared_mutex> childLk{curNode->mut};
        lk = std::move(childLk);
      }
    }
  }

  bool insert(const T& key) {
    std::unique_lock<std::shared_mutex> lk{root->mut};
    FGLBSTNode<T>* cur = root, *child = root->left;

    while (true) {
      std::unique_lock<std::shared_mutex> childLk{child->mut};
      if (child->key == key) return false;
      else if (key < child->key) {
        if (child->left == nullptr) break;
        cur = child;
        child = child->left;
        lk = std::move(childLk);
      } else {
        if (child->right == nullptr) break;
        cur = child;
        child = child->right;
        lk = std::move(childLk);
      }
    }

    if (key < child->key) child->left = new FGLBSTNode<T>(key);
    else child->right = new FGLBSTNode<T>(key);
    return true;
  }

  bool remove(const T& key) {
    std::unique_lock<std::shared_mutex> lk{root->mut}, deleteLk;
    FGLBSTNode<T>* cur = root, *child = root->left;

    while (true) {
      std::unique_lock<std::shared_mutex> childLk{child->mut};
      if (child->key == key) {
        deleteLk = std::move(childLk);
        break;
      } else if (key < child->key) {
        if (child->left == nullptr) return false;
        cur = child;
        child = child->left;
        lk = std::move(childLk);
      } else {
        if (child->right == nullptr) return false;
        cur = child;
        child = child->right;
        lk = std::move(childLk);
      }
    }

    // Three Cases:
    // 1. No children
    // 2. One child
    if (child->left == nullptr || child->right == nullptr) {
      FGLBSTNode<T> *sucessorNode = child->left == nullptr ? child->right : child->left;
      if (cur->left == child) cur->left = sucessorNode;
      else cur->right = sucessorNode;

      return true;
    }

    // 3. There must be 2 children
    std::unique_lock<std::shared_mutex> inorderParentLk, inorderSuccessorLk{child->left->mut};
    FGLBSTNode<T> **inorderSuccessorPtr = &(child->left);
    FGLBSTNode<T> *inorderParent = child, *inorderSuccessor = child->left;
    while (inorderSuccessor->right != nullptr) {
      inorderParent = inorderSuccessor;
      inorderSuccessorPtr = &(inorderSuccessor->right);
      inorderSuccessor = inorderSuccessor->right;

      std::unique_lock<std::shared_mutex> grandChildLk{inorderSuccessor->mut};
      inorderParentLk = std::move(inorderSuccessorLk);
      inorderSuccessorLk = std::move(grandChildLk);
    }

    child->key = inorderSuccessor->key;
    *inorderSuccessorPtr = inorderSuccessor->left;
    return true;
  }


};