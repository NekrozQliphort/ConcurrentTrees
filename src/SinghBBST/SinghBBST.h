#pragma once

#include <atomic>
#include <utility>
#include <thread>
#include <iostream>

#include "Node.h"
#include "Operation.h"
#include "SeekRecord.h"

template <class T, T inf = std::numeric_limits<T>::max()>
struct SinghBBST {
  static Node<T>* const sentinel; // For swapping purposes

  SinghBBST() {
    // Init here to make sure all other fields are initialized
    maintainenceThread = std::thread(&SinghBBST<T>::maintain, this, root);
  }

  ~SinghBBST() {
    finished.store(true);
    if (maintainenceThread.joinable()) maintainenceThread.join();
    cleanup(root);
  }

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
      return getFlag(nodeOp) == OperationConstants::INSERT &&
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

      Operation<T>* casOp =
          new Operation<T>(std::in_place_type<InsertOp<T>>, isLeft,
                           result.result == SeekResultState::FOUND &&
                               result.node->deleted.load(),
                           old, newNode);
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

  static const OperationFlaggedPointer NULLOFP =
      reinterpret_cast<OperationFlaggedPointer>(nullptr);
  std::atomic<bool> finished {0};
  std::thread maintainenceThread;

  enum class HeightBalanceState {
    NO_ROTATION,  // Height diff <= 1
    LEFT_ROTATE,  // Height diff >= 2
    RIGHT_ROTATE,
    FORCE_LEFT_ROTATE,  // Height diff >= 2, require double rotate, should force X child to rotate
    FORCE_RIGHT_ROTATE,
  };

  void helpRotate(Operation<T>* op, Node<T>* parent, Node<T>* node,
                  Node<T>* child) {
    RotateOp<T>& rotateOp = get<RotateOp<T>>(*op);

    for (int seen_state = rotateOp.state.load();
         seen_state != RotateOp<T>::DONE; seen_state = rotateOp.state.load()) {
      if (seen_state == RotateOp<T>::UNDECIDED) {  // Grab First Node
        OperationFlaggedPointer nodeOp = node->op.load();
        OperationConstants::Flags currentFlag = getFlag(nodeOp);
        if (currentFlag == OperationConstants::Flags::ROTATE) {
          int expected = RotateOp<T>::UNDECIDED,
              desired = RotateOp<T>::GRABBED_FIRST;
          rotateOp.state.compare_exchange_strong(expected, desired);
        } else if (currentFlag == OperationConstants::INSERT) {
          help(nullptr, NULLOFP, node,
               nodeOp);  // First 2 arguments don't matter for insert
        } else if (currentFlag == OperationConstants::NONE) {
          OperationFlaggedPointer expected = nodeOp,
                                  desired = flag(
                                      op, OperationConstants::Flags::ROTATE);
          // No need extra checks whether it is decided or not, can only happen once (node never gets set back to NONE)
          node->op.compare_exchange_strong(expected, desired);
        } else
          std::unreachable();  // Shouldnt be marked, should be handled already

      } else if (seen_state == RotateOp<T>::GRABBED_FIRST) {

        OperationFlaggedPointer childOp = child->op.load();
        OperationConstants::Flags currentFlag = getFlag(childOp);
        if (currentFlag == OperationConstants::Flags::ROTATE) {
          int expected = RotateOp<T>::GRABBED_FIRST,
              desired = RotateOp<T>::GRABBED_SECOND;
          rotateOp.state.compare_exchange_strong(expected, desired);
        } else if (currentFlag == OperationConstants::INSERT) {
          help(nullptr, NULLOFP, child,
               childOp);  // First 2 arguments don't matter for insert
        } else if (currentFlag == OperationConstants::NONE) {
          OperationFlaggedPointer expectedOp = childOp,
                                  desiredOp = flag(
                                      op, OperationConstants::Flags::ROTATE);
          Node<T>*expectedNode = sentinel,
          *desiredNode = rotateOp.isLeftRotation ? child->left.load()
                                                 : child->right.load();
          // Require extra checks as childOp might be None AFTER the rotationOperation is done completely
          // Eg. Interrupted and some other thread finished the rotation then an insertion happens is possible
          if (rotateOp.state.load() != RotateOp<T>::GRABBED_FIRST)
            continue;
          // This comes first as change to rotate state should not be observed until node is first swapped in
          rotateOp.grandchild.compare_exchange_strong(expectedNode,
                                                      desiredNode);
          child->op.compare_exchange_strong(
              expectedOp,
              desiredOp);  // Success means rotateOp has not progressed beyond GRABBED_FIRST
        } else
          std::unreachable();  // Shouldnt be marked, should be handled already

      } else if (seen_state == RotateOp<T>::GRABBED_SECOND) {
        // Create correct node to prepare for insertion and CAS newNode
        Node<T>* expected = rotateOp.grandchild.load();
        if (rotateOp.isLeftRotation) {
          Node<T>* newNode = new Node<T>{
              node->key,    node->left.load(), rotateOp.grandchild.load(),
              node->lh,     node->rh,          node->deleted.load(),
              node->removed.load()};
          newNode->op.store(flag(op, OperationConstants::ROTATE));
          if (!child->left.compare_exchange_strong(expected, newNode))
            delete newNode;  // Only happens successfully once

        } else {
          Node<T>* newNode =
              new Node<T>{node->key,          rotateOp.grandchild.load(),
                          node->right.load(), node->lh,
                          node->rh,           node->deleted.load(),
                          node->removed.load()};
          newNode->op.store(flag(op, OperationConstants::ROTATE));
          if (!child->right.compare_exchange_strong(expected, newNode))
            delete newNode;  // Only happens successfully once
        }

        // Final CAS for parent to swap to correct node
        if (Node<T>* expected = node, *desired = child; rotateOp.isLeftChild) {
          parent->left.compare_exchange_strong(expected, desired);
        } else {
          parent->right.compare_exchange_strong(expected, desired);
        }

        int expectedState = RotateOp<T>::GRABBED_SECOND,
            desiredState = RotateOp<T>::ROTATED;
        rotateOp.state.compare_exchange_strong(expectedState, desiredState);
      } else if (seen_state == RotateOp<T>::ROTATED) {
        OperationFlaggedPointer expected,
            desired = flag(op, OperationConstants::NONE);
        parent->op.compare_exchange_strong(
            expected = flag(op, OperationConstants::ROTATE), desired);
        child->op.compare_exchange_strong(
            expected = flag(op, OperationConstants::ROTATE), desired);
        Node<T>* newNode =
            rotateOp.isLeftRotation ? child->left.load() : child->right.load();
        newNode->op.compare_exchange_strong(
            expected = flag(op, OperationConstants::ROTATE), desired);

        int expectedState = RotateOp<T>::ROTATED,
            desiredState = RotateOp<T>::DONE;
        rotateOp.state.compare_exchange_strong(expectedState, desiredState);
      }
    }
  }

  HeightBalanceState checkBalance(Node<T>* node, bool forced) {
    if (node->rh - node->lh >= 2 - forced)
      return HeightBalanceState::LEFT_ROTATE;
    else if (node->lh - node->rh >= 2 - forced)
      return HeightBalanceState::RIGHT_ROTATE;
    else
      return HeightBalanceState::NO_ROTATION;
  }

  HeightBalanceState leftRotate(Node<T>* parent, bool isLeftChild,
                                bool forced) {
    // Assumption: Only called when it is already confirmed to be imbalanced
    if (parent->removed.load())
      return HeightBalanceState::NO_ROTATION;

    Node<T>*current = isLeftChild ? parent->left.load() : parent->right.load(),
    *child = nullptr;
    if (current == nullptr || (child = current->right.load()) == nullptr)
      return HeightBalanceState::NO_ROTATION;

    if (child->lh - child->rh >= 1 && !forced)
      return HeightBalanceState::FORCE_RIGHT_ROTATE;

    // All checking done, attempt to swap rotation intention in
    OperationFlaggedPointer parentOp = parent->op.load();
    if (getFlag(parentOp) == OperationConstants::NONE) {
      Operation<T>* rotationOp =
          new Operation<T>(std::in_place_type<RotateOp<T>>, parent, current,
                           child, true, isLeftChild, sentinel);

      if (parent->op.compare_exchange_strong(
              parentOp, flag(rotationOp, OperationConstants::ROTATE))) {
        helpRotate(rotationOp, parent, current, child);
        return HeightBalanceState::LEFT_ROTATE;
      } else {
        delete rotationOp;
        return HeightBalanceState::NO_ROTATION;
      }
    }
    return HeightBalanceState::NO_ROTATION;
  }

  HeightBalanceState rightRotate(Node<T>* parent, bool isLeftChild,
                                 bool forced) {
    // Assumption: Only called when it is already confirmed to be imbalanced
    if (parent->removed.load())
      return HeightBalanceState::NO_ROTATION;

    Node<T>*current = isLeftChild ? parent->left.load() : parent->right.load(),
    *child = nullptr;
    if (current == nullptr || (child = current->left.load()) == nullptr)
      return HeightBalanceState::NO_ROTATION;

    if (child->rh - child->lh >= 1 && !forced)
      return HeightBalanceState::FORCE_LEFT_ROTATE;

    // All checking done, attempt to swap rotation intention in
    OperationFlaggedPointer parentOp = parent->op.load();
    if (getFlag(parentOp) == OperationConstants::NONE) {
      Operation<T>* rotationOp =
          new Operation<T>(std::in_place_type<RotateOp<T>>, parent, current,
                           child, false, isLeftChild, sentinel);
      if (parent->op.compare_exchange_strong(
              parentOp, flag(rotationOp, OperationConstants::ROTATE))) {
        helpRotate(rotationOp, parent, current, child);
        return HeightBalanceState::RIGHT_ROTATE;
      } else {
        delete rotationOp;
        return HeightBalanceState::NO_ROTATION;
      }
    }
    return HeightBalanceState::NO_ROTATION;
  }

  void cleanup(Node<T>* node) {
    if (node == nullptr) return;
    cleanup(node->left.load());
    cleanup(node->right.load());

    delete node;
  }

  int maintainHelper(Node<T>* node, Node<T>* parent, bool isLeftChild, bool forced) {
    if (node == nullptr) return 0;
    if (!forced) node->lh = maintainHelper(node->left.load(), node, true, false);
    if (!forced) node->rh = maintainHelper(node->right.load(), node, false, false);
    node->local_height = std::max(node->lh, node->rh) + 1;

    HeightBalanceState state = checkBalance(node, forced);
    if (state == HeightBalanceState::NO_ROTATION) return node->local_height;
    else if (state == HeightBalanceState::LEFT_ROTATE) {
      state = leftRotate(parent, isLeftChild, forced);
      if (state == HeightBalanceState::FORCE_RIGHT_ROTATE) {
        node->lh = maintainHelper(node->right.load(), node, false, true);
        leftRotate(parent, isLeftChild, false);
      }
    } else {
      state = rightRotate(parent, isLeftChild, forced);
      if (state == HeightBalanceState::FORCE_LEFT_ROTATE) {
        node->rh = maintainHelper(node->left.load(), node, true, true);
        rightRotate(parent, isLeftChild, false);
      }
    }

    if (state != HeightBalanceState::NO_ROTATION) node->local_height--;
    return node->local_height;
  }

  void maintain(Node<T>* root) {
    while (!finished.load()) {
      maintainHelper(root->left.load(), root, true, false);
    }
  }

  void helpMarked(Operation<T>* parentOp, Node<T>* parent, Node<T>* node) {
    Node<T>* child = node->left.load();
    if (child == nullptr)
      node->right.load();

    node->removed = true;
    Operation<T>* casOp = new Operation<T>(std::in_place_type<InsertOp<T>>,
                                           node == parent->left, node, child);
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
    if (getFlag(nodeOp) == OperationConstants::INSERT) {
      helpInsert(unFlag<T>(nodeOp), node);
    } else if (getFlag(parentOp) == OperationConstants::ROTATE) {
      Operation<T>* actualOp = unFlag<T>(parentOp);
      RotateOp<T>& actualRotateOp = get<RotateOp<T>>(*actualOp);
      helpRotate(actualOp, actualRotateOp.parent, actualRotateOp.node,
                 actualRotateOp.child);
    } else if (getFlag(nodeOp) == OperationConstants::MARK) {
      helpMarked(reinterpret_cast<Operation<T>*>(parentOp), parent, node);
    }
  }

  SeekRecord<T> seek(const T& key, Node<T>* auxRoot, Node<T>* root) {
    SeekRecord<T> res{};
    T nodeKey;
    Node<T>* nxt;

  retry:
    res.result = SeekResultState::NOT_FOUND_L;
    res.node = root;
    res.nodeOp = res.node->op.load();

    if (getFlag(res.nodeOp) == OperationConstants::INSERT) {
      helpInsert(unFlag<T>(res.nodeOp), res.node);
      goto retry;
    } else if (getFlag(res.nodeOp) == OperationConstants::ROTATE) {
      help(res.node, res.nodeOp, nullptr, NULLOFP);
      goto retry;
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

template <class T, T inf>
inline Node<T>* const SinghBBST<T, inf>::sentinel = new Node<T>(T{inf});

template struct SinghBBST<int>;