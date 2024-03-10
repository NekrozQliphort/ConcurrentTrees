#pragma once

#include <atomic>
#include <utility>
#include "Node.h"
#include "Operation.h"
#include "SeekRecord.h"

template <class T, T inf = std::numeric_limits<T>::max()>
struct SinghBBST {
  bool operator[](const T& k) {
    Node<T>*node = root, *nxt = root->left.load();
    OperationFlaggedPointer nodeOp = 0;
    T nodeKey;
    bool result = false;

    while (nxt != nullptr && !result) {
      node = nxt;
      nodeOp = node->op.load();
      nodeKey = node->key;
      if (k < nodeKey)
        nxt = node->left.load();
      else if (k > nodeKey)
        nxt = node->right.load();
      else
        result = true;
    }

    if (result && node->deleted.load()) {
      return getFlag(node->op) == OperationConstants::INSERT &&
             get<InsertOp<T>>(*getPointer<T>(nodeOp)).newNode->key == k;
    }
    return result;
  }

  bool insert(const T& key) {
    Node<T>* newNode{nullptr};
    while (true) {
      SeekRecord<T> result = seek(key, root, root);
      if (result.result == SeekResultState::FOUND &&
          !result.node->deleted.load())
        return false;
      if (newNode == nullptr)
        newNode = new Node<T>(key);

      bool isLeft = (result.result == SeekResultState::NOT_FOUND_L);
      Node<T>* old =
          isLeft ? result.node->left.load() : result.node->right.load();

      Operation<T>* casOp = new Operation<T>(
          InsertOp<T>{.isLeft = isLeft,
                      .isUpdate = result.result == SeekResultState::FOUND &&
                                  result.node->deleted.load(),
                      .expectedNode = old,
                      .newNode = newNode});

      if (result.node->op.compare_exchange_strong(
              result.nodeOp, flag(casOp, OperationConstants::INSERT))) {
        helpInsert(casOp, result.node);
        return true;
      }
    }

    std::unreachable();
  }

  bool remove(const T& key) {
    while (true) {
      SeekRecord<T> result = seek(key, root, root);
      if (result.result != SeekResultState::FOUND)
        return false;
      if (result.node->deleted.load()) {
        if (getFlag(result.node->op.load()) != OperationConstants::INSERT)
          return false;
      } else {
        if (getFlag(result.node->op.load()) == OperationConstants::NONE) {
          bool expected = false, desired = true;
          if (result.node->deleted.compare_exchange_strong(expected, desired)) {
            return true;
          }
        }
      }
    }
  }

 private:
  Node<T>* root = new Node<T>(T{inf});

  void helpMarked(Node<T>* parent, Operation<T>* parentOp, Node<T>* node) {
    Node<T>* child = node->left.load();
    if (child == nullptr)
      node->right.load();

    node->removed = true;
    Operation<T>* casOp =
        new Operation<T>(InsertOp<T>{.isLeft = node == parent->left,
                                     .expectedNode = node,
                                     .newNode = child});

    OperationFlaggedPointer expected =
        reinterpret_cast<OperationFlaggedPointer>(parentOp);
    if (parent->op.compare_exchange_strong(
            expected, flag(casOp, OperationConstants::INSERT))) {
      helpInsert(casOp, parent);
    }
  }

  void helpInsert(Operation<T>* op, Node<T>* dest) {
    // TODO: Assumed op to be unflagged
    InsertOp<T>& insertOp = get<InsertOp<T>>(*op);
    if (insertOp.isUpdate) {
      bool expected = true, desired = false;
      dest->deleted.compare_exchange_strong(expected, desired);
    } else {
      std::atomic<Node<T>*>& addr = insertOp.isLeft ? dest->left : dest->right;
      addr.compare_exchange_strong(insertOp.expectedNode, insertOp.newNode);
    }

    OperationFlaggedPointer expected = flag(op, OperationConstants::INSERT),
                            desired = flag(op, OperationConstants::NONE);
    dest->op.compare_exchange_strong(expected, desired);
  }

  void help(Node<T>* parent, OperationFlaggedPointer parentOp, Node<T>* node,
            OperationFlaggedPointer nodeOp) {
    // TODO: Implement
    if (getFlag(nodeOp) == OperationConstants::INSERT) {
      helpInsert(unFlag<T>(nodeOp), node);
    } else if (getFlag(nodeOp) == OperationConstants::MARK) {
      helpMarked(parent, reinterpret_cast<Operation<T>*>(parentOp), node);
    }
  }

  SeekRecord<T> seek(const T& key, Node<T>* auxRoot, Node<T>* root) {
    SeekRecord<T> res{};
    T nodeKey;
    Node<T>* nxt;

  retry:
    res.result = SeekResultState::NOT_FOUND_L;
    res.node = auxRoot;
    res.nodeOp = res.node->op.load();

    if (getFlag(res.nodeOp) != OperationConstants::NONE) {
      if (auxRoot == root) {
        helpInsert(unFlag<T>(res.nodeOp), res.node);
        goto retry;
      }
    }

    nxt = res.node->left.load();
    while (nxt != nullptr && res.result != SeekResultState::FOUND) {
      res.parent = res.node;
      res.parentOp = res.nodeOp;
      res.node = nxt;
      res.nodeOp = res.node->op.load();
      nodeKey = res.node->key;

      if (key < nodeKey) {
        res.result = SeekResultState::NOT_FOUND_L;
        nxt = res.node->left.load();
      } else if (key > nodeKey) {
        res.result = SeekResultState::NOT_FOUND_R;
        nxt = res.node->right.load();
      } else {
        res.result = SeekResultState::FOUND;
      }
    }

    if (getFlag(res.nodeOp) != OperationConstants::NONE) {
      help(res.parent, res.parentOp, res.node, res.nodeOp);
      goto retry;
    }
    return res;
  }
};

template struct SinghBBST<int>;