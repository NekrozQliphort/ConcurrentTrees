#pragma once

#include <algorithm>
#include <atomic>
#include <bit>
#include <limits>

#include "Node.h"
#include "SeekRecord.h"

template <class T, T inf0 = std::numeric_limits<T>::max() - 2,
          T inf1 = std::numeric_limits<T>::max() - 1,
          T inf2 = std::numeric_limits<T>::max()>
struct NatarajanBST {
  using NPF = typename Node<T>::NodePtrWithFlags;
  using NPData = typename Node<T>::Data;

  constexpr static uint32_t FLAG = 2, TAG = 1;
  constexpr static NPData FLAG_MASK =
      NPData(2) << (std::endian::native == std::endian::big ? 32 : 64);
  constexpr static NPData TAG_MASK =
      NPData(1) << (std::endian::native == std::endian::big ? 32 : 64);

  enum class DeleteMode { INJECTION, CLEANUP };

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
    while (true) {
      SeekRecord<T> s = seek(key);
      // key already in tree
      if (s.leaf->key == key)
        return false;
      Node<T>*parent = s.parent, *leaf = s.leaf, *newLeaf = new Node<T>(key);
      decltype(parent->left)* childAddr;

      if (key < parent->key)
        childAddr = &(parent->left);
      else
        childAddr = &(parent->right);

      Node<T>*l = newLeaf, *r = std::bit_cast<NPF>(childAddr->load()).ptr;
      if (l->key > r->key)
        std::swap(l, r);

      Node<T>* newInternal = new Node(r->key, l, r);
      NPF expected(leaf), desired(newInternal);
      NPData expected_cast = std::bit_cast<NPData>(expected),
             desired_cast = std::bit_cast<NPData>(desired);
      if (childAddr->compare_exchange_strong(expected_cast, desired_cast))
        return true;

      const auto c = std::bit_cast<NPF>(childAddr->load());
      if (c.ptr == leaf && (c.flags != 0)) {
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
      std::atomic<NPData>* childAddr =
          key < s.parent->key ? &(s.parent->left) : &(s.parent->right);
      if (mode == DeleteMode::INJECTION) {
        leaf = s.leaf;
        if (leaf->key != key)
          return false;
        NPData expected_cast = std::bit_cast<NPData>(NPF{leaf}),
               desired_cast = std::bit_cast<NPData>(NPF{leaf, FLAG});
        if (childAddr->compare_exchange_strong(expected_cast, desired_cast)) {
          mode = DeleteMode::CLEANUP;
          if (cleanup(key, s))
            return true;
        } else {
          NPF data = std::bit_cast<NPF>(childAddr->load());
          if (data.ptr == leaf && (data.flags != 0))
            cleanup(key, s);
        }
      } else {
        if (s.leaf != leaf || cleanup(key, s))
          return true;
      }
    }
  }

 private:
  SeekRecord<T> seek(const T& key) {
    SeekRecord<T> s;
    s.ancestor = root;
    s.successor = std::bit_cast<NPF>(root->left.load()).ptr;
    s.parent = s.successor;
    s.leaf = std::bit_cast<NPF>(s.successor->left.load()).ptr;
    NPF parentField = std::bit_cast<NPF>(s.parent->left.load()),
        currentField = std::bit_cast<NPF>(s.leaf->left.load());
    for (auto current = currentField.ptr; current != nullptr;) {
      if (!(parentField.flags & TAG_MASK)) {
        s.ancestor = s.parent;
        s.successor = s.leaf;
      }
      s.parent = s.leaf;
      s.leaf = current;

      parentField = currentField;
      if (key < current->key) {
        currentField = std::bit_cast<NPF>(current->left.load());
      } else
        currentField = std::bit_cast<NPF>(current->right.load());
      current = currentField.ptr;
    }
    return s;
  }

  bool cleanup(const T& key, const SeekRecord<T>& s) {
    const auto [ancestor, successor, parent, leaf] = s;
    std::atomic<NPData>*successorAddr, *childAddr, *siblingAddr;

    childAddr = &(parent->right);
    siblingAddr = &(parent->left);

    if (key < ancestor->key)
      successorAddr = &(ancestor->left);
    else
      successorAddr = &(ancestor->right);

    if (key < parent->key)
      std::swap(childAddr, siblingAddr);

    if ((childAddr->load() & FLAG_MASK) == 0) {
      siblingAddr = childAddr;
    }

    NPData siblingVal = siblingAddr->fetch_or(TAG) & (~TAG_MASK);
    NPData expected = std::bit_cast<NPData>(NPF{successor});
    return successorAddr->compare_exchange_strong(expected, siblingVal);
  }

  void cleanup_all(Node<T>* node) {
    if (node == nullptr)
      return;
    cleanup_all(std::bit_cast<NPF>(node->left.load()).ptr);
    cleanup_all(std::bit_cast<NPF>(node->right.load()).ptr);
    delete node;
  }
};