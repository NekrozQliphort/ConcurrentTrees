#pragma once

#include <atomic>
#include "Node.h"
#include "Operation.h"
#include "SeekRecord.h"

template <class T>
struct SinghBBST {
  bool operator[](const T& k) {
    Node<T>*node = root, *nxt = root->right.load();
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

    if (result && node->deleted) {
      return getFlag(node->op) == OperationConstants::INSERT &&
             get<InsertOp<T>>(*getPointer<T>(nodeOp)).newNode->key == k;
    }
    return false;
  }

 private:
  Node<T>* root = new Node<T>(T{});

  void helpInsert(Operation<T>* op, Node<T>* dest) {
    // TODO: Implement
  }

  void help(Node<T>* parent, std::atomic<OperationFlaggedPointer>* parentOp,
            Node<T>* node, std::atomic<OperationFlaggedPointer>* nodeOp) {
    // TODO: Implement
  }

  SeekRecord<T> seek(const T& key, Node<T>* auxRoot, Node<T>* root) {
    SeekRecord<T> res{};
    T nodeKey;
    Node<T>* nxt;

  retry:
    res.result = SeekResultState::NOT_FOUND_L;
    res.node = root;
    res.nodeOp = &res.node->op;

    if (getFlag(*res.nodeOp) != OperationConstants::NONE) {
      if (auxRoot == root) {
        helpInsert(unFlag<T>(*res.nodeOp), res.node);
        goto retry;
      }
    }

    nxt = res.node->right.load();
    while (nxt != nullptr) {
      res.parent = res.node;
      res.parentOp = res.nodeOp;
      res.node = nxt;
      res.nodeOp = &res.node->op;
      nodeKey = res.node->key;

      if (key < nodeKey) {
        res.result = SeekResultState::NOT_FOUND_L;
        nxt = res.node->left.load();
      } else if (key > nodeKey) {
        res.result = SeekResultState::NOT_FOUND_R;
        nxt = res.node->right.load();
      } else {
        res.result = SeekResultState::FOUND;
        break;
      }
    }

    if (getFlag(res.nodeOp->load()) != OperationConstants::NONE) {
      help(res.parent, res.parentOp, res.node, res.parentOp);
      goto retry;
    }
    return res;
  }
};