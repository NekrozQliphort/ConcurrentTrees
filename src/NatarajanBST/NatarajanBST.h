#pragma once

#include <atomic>
#include <limits>

#include "Node.h"
#include "SeekRecord.h"

template <class T, T inf0 = std::numeric_limits<T>::max() - 2,
          T inf1 = std::numeric_limits<T>::max() - 1,
          T inf2 = std::numeric_limits<T>::max()>
struct NatarajanBST {
 public:
  Node<T>* root;

  NatarajanBST() {
    auto* S = new Node<T>(inf1, new Node<T>(inf0), new Node<T>(inf1));
    root = new Node<T>(inf2, S, new Node<T>(inf2));
  }

  ~NatarajanBST() { cleanup_all(root); }

  bool operator[](const T& key) {
    SeekRecord<T> s = seek(key);
    return s.leaf->key == key;
  }

  bool insert(const T& key) {
    auto *newLeaf = new Node<T>(key), *newInternal = new Node<T>(key);
    assert((reinterpret_cast<uintptr_t>(newLeaf) & Node<T>::FLAG_MASK) == 0);
    assert((reinterpret_cast<uintptr_t>(newLeaf) & Node<T>::FLAG_MASK) == 0);

    while (true) {
      SeekRecord<T> s = seek(key);
      // key already in tree
      if (s.leaf->key == key) {
        delete newLeaf;
        delete newInternal;
        return false;
      }

      Node<T>*parent = s.parent, *leaf = s.leaf;
      std::atomic<uintptr_t>* childAddr =
          key < parent->key ? &(parent->left) : &(parent->right);

      Node<T>*l = newLeaf, *r = getPointer<T>(childAddr->load());

      if (l->key > r->key)
        std::swap(l, r);

      newInternal->key = r->key;
      newInternal->left.store(reinterpret_cast<uintptr_t>(l));
      newInternal->right.store(reinterpret_cast<uintptr_t>(r));

      uintptr_t expected = getPointerUintRepr(leaf),
                desired = getPointerUintRepr(newInternal);

      if (childAddr->compare_exchange_strong(expected, desired))
        return true;

      const auto c = childAddr->load();
      if (getPointer<T>(c) == leaf && getFlags<T>(c) != 0) {
        // cleanup
        cleanup(key, s);
      }
    }
  }

  bool remove(const T& key) {
    DeleteMode mode = DeleteMode::INJECTION;
    Node<T>* leaf;
    while (true) {
      SeekRecord<T> s = seek(key);
      std::atomic<uintptr_t>* childAddr =
          key < s.parent->key ? &(s.parent->left) : &(s.parent->right);
      if (mode == DeleteMode::INJECTION) {
        leaf = s.leaf;
        if (leaf->key != key)
          return false;
        uintptr_t expected = getPointerUintRepr<T>(leaf),
                  desired = expected | Node<T>::FLAG_MASK;
        if (childAddr->compare_exchange_strong(expected, desired)) {
          mode = DeleteMode::CLEANUP;
          if (cleanup(key, s)) {
            return true;
          }
        } else {
          uintptr_t childData = childAddr->load();
          if (getPointer<T>(childData) == leaf && getFlags<T>(childData) != 0)
            cleanup(key, s);
        }
      } else {
        if (s.leaf != leaf || cleanup(key, s))
          return true;
      }
    }
  }

 private:
  enum class DeleteMode { INJECTION, CLEANUP };

  SeekRecord<T> seek(const T& key) {
    SeekRecord<T> s;
    s.ancestor = root;
    s.successor = reinterpret_cast<Node<T>*>(root->left.load());
    s.parent = s.successor;
    s.leaf = getPointer<T>(s.successor->left.load());

    uintptr_t parentField = s.parent->left.load(),
              currentField = s.leaf->left.load();

    // Assumption: nullptr will be 0, use NULL?
    for (Node<T>* current = getPointer<T>(currentField); current != nullptr;
         current = getPointer<T>(currentField)) {
      if (!(reinterpret_cast<uintptr_t>(parentField) & Node<T>::TAG_MASK)) {
        s.ancestor = s.parent;
        s.successor = s.leaf;
      }
      s.parent = s.leaf;
      s.leaf = current;
      parentField = currentField;

      if (key < current->key)
        currentField = current->left.load();
      else
        currentField = current->right.load();
    }

    return s;
  }

  bool cleanup(const T& key, const SeekRecord<T>& s) {
    const auto [ancestor, successor, parent, leaf] = s;
    std::atomic<uintptr_t>*successorAddr =
        key < ancestor->key ? &(ancestor->left) : &(ancestor->right),
    *childAddr = &(parent->right), *siblingAddr = &(parent->left);

    if (key < parent->key)
      std::swap(childAddr, siblingAddr);

    if ((childAddr->load() & Node<T>::FLAG_MASK) == 0)
      siblingAddr = childAddr;

    uintptr_t siblingData =
        siblingAddr->fetch_or(Node<T>::TAG_MASK) & (~Node<T>::TAG_MASK);
    uintptr_t expected = getPointerUintRepr(successor);  // Remove all flags
    return successorAddr->compare_exchange_strong(expected, siblingData);
  }

  void cleanup_all(Node<T>* node) {
    if (node == nullptr)
      return;
    cleanup_all(getPointer<T>(node->left.load()));
    cleanup_all(getPointer<T>(node->right.load()));
    delete node;
  }
};