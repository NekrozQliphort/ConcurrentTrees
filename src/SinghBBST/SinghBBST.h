#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <utility>

#include "src/SinghBBST/Node.h"
#include "src/SinghBBST/Operation.h"
#include "src/SinghBBST/SeekRecord.h"

template <class T, T inf = std::numeric_limits<T>::max()>
struct SinghBBST {
  static Singh::Node<T>* const sentinel;  // For swapping purposes

  SinghBBST() {
    // Init here to make sure all other fields are initialized
    maintainenceThread = std::thread(&SinghBBST<T>::maintain, this, root);
  }

  ~SinghBBST() {
    finished.store(true);
    if (maintainenceThread.joinable())
      maintainenceThread.join();
    cleanup(root);
  }

  bool operator[](const T& k) {
    Singh::Node<T>*node = root, *nxt = root->left.load();
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

    if (result && (node->deleted.load() & 1) == 1) {
      return getFlag(nodeOp) == OperationConstants::INSERT &&
             get<InsertOp<T>>(*Singh::getPointer<T>(nodeOp)).newNode->key == k;
    }
    return result;
  }

  bool insert(const T& key) {
    Singh::Node<T>* newNode{nullptr};
    while (true) {
      Singh::SeekRecord<T> result = seek(key);
      if (result.result == SeekResultState::FOUND &&
          (result.node->deleted.load() & 1) == 0)
        return false;
      if (newNode == nullptr)
        newNode = new Singh::Node<T>(key);

      bool isLeft = (result.result == SeekResultState::NOT_FOUND_L);
      Singh::Node<T>* old =
          isLeft ? result.node->left.load() : result.node->right.load();

      Operation<T>* casOp =
          new Operation<T>(std::in_place_type<InsertOp<T>>, isLeft,
                           result.result == SeekResultState::FOUND &&
                               (result.node->deleted.load() & 1) ==
                                   1,  // TODO: Might need another fix
                           old, newNode);
      if (result.node->op.compare_exchange_strong(
              result.nodeOp, Singh::flag(casOp, OperationConstants::INSERT))) {
        helpInsert(casOp, result.node);
        return true;
      }
    }

  }

  bool remove(const T& key) {
    while (true) {
      Singh::SeekRecord<T> result = seek(key);
      if (result.result != SeekResultState::FOUND)
        return false;
      if ((result.node->deleted.load() & 1) == 1) {
        if (getFlag(result.node->op.load()) != OperationConstants::INSERT)
          return false;
      } else {
        if (getFlag(result.node->op.load()) == OperationConstants::NONE) {
          uint8_t expected = 0, desired = 1;
          if (result.node->deleted.compare_exchange_strong(expected, desired)) {
            return true;
          }
        }
      }
    }
  }

 private:
  Singh::Node<T>* root = new Singh::Node<T>(T{inf});

  static const OperationFlaggedPointer NULLOFP =
      reinterpret_cast<OperationFlaggedPointer>(nullptr);
  std::atomic<bool> finished{false};
  std::thread maintainenceThread;

  enum class HeightBalanceState {
    NO_ROTATION,  // Height diff <= 1
    LEFT_ROTATE,  // Height diff >= 2
    RIGHT_ROTATE,
    FORCE_LEFT_ROTATE,  // Height diff >= 2, require double rotate, should force X child to rotate
    FORCE_RIGHT_ROTATE,
  };

  void helpRotate(Operation<T>* op, Singh::Node<T>* parent,
                  Singh::Node<T>* node, Singh::Node<T>* child) {
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
                                  desired = Singh::flag(
                                      op, OperationConstants::Flags::ROTATE);
          // No need extra checks whether it is decided or not, can only happen once (node never gets set back to NONE)
          node->op.compare_exchange_strong(expected, desired);
        }

      } else if (seen_state == RotateOp<T>::GRABBED_FIRST) {

        OperationFlaggedPointer childOp = child->op.load();
        OperationConstants::Flags currentFlag = getFlag(childOp);
        if (currentFlag == OperationConstants::Flags::ROTATE) {
          Singh::Node<T>*expectedNode = sentinel,
          *desiredNode = rotateOp.isLeftRotation ? child->left.load()
                                                 : child->right.load();
          rotateOp.grandchild.compare_exchange_strong(expectedNode,
                                                      desiredNode);
          int expected = RotateOp<T>::GRABBED_FIRST,
              desired = RotateOp<T>::GRABBED_SECOND;
          rotateOp.state.compare_exchange_strong(expected, desired);
        } else if (currentFlag == OperationConstants::INSERT) {
          help(nullptr, NULLOFP, child,
               childOp);  // First 2 arguments don't matter for insert
        } else if (currentFlag == OperationConstants::NONE) {
          OperationFlaggedPointer expectedOp = childOp,
                                  desiredOp = Singh::flag(
                                      op, OperationConstants::Flags::ROTATE);
          // Require extra checks as childOp might be None AFTER the rotationOperation is done completely
          // Eg. Interrupted and some other thread finished the rotation then an insertion happens is possible
          if (rotateOp.state.load() != RotateOp<T>::GRABBED_FIRST)
            continue;
          child->op.compare_exchange_strong(
              expectedOp,
              desiredOp);  // Success means rotateOp has not progressed beyond GRABBED_FIRST
        }

      } else if (seen_state == RotateOp<T>::GRABBED_SECOND) {
        // Create correct node to prepare for insertion and CAS newNode
        Singh::Node<T>* expected = rotateOp.grandchild.load();
        Singh::Node<T>* newNode;
        if (rotateOp.isLeftRotation) {
          uint8_t deleted = node->deleted.fetch_or(2) &
                            1u;  // Make sure it's unusable for remove
          newNode = new Singh::Node<T>{
              node->key, node->left.load(), rotateOp.grandchild.load(), 0, 0,
              0,         deleted,           node->removed.load()};
          newNode->op.store(Singh::flag(op, OperationConstants::ROTATE));
          if (!child->left.compare_exchange_strong(expected, newNode))
            delete newNode;  // Only happens successfully once
        } else {
          uint8_t deleted = node->deleted.fetch_or(2) & 1u;
          newNode = new Singh::Node<T>{node->key,
                                       rotateOp.grandchild.load(),
                                       node->right.load(),
                                       0,
                                       0,
                                       0,
                                       deleted,
                                       node->removed.load()};
          newNode->op.store(Singh::flag(op, OperationConstants::ROTATE));
          if (!child->right.compare_exchange_strong(expected, newNode))
            delete newNode;  // Only happens successfully once
        }

        // Final CAS for parent to swap to correct node
        if (Singh::Node<T>* expected = node, *desired = child;
            rotateOp.isLeftChild) {
          parent->left.compare_exchange_strong(expected, desired);
        } else {
          parent->right.compare_exchange_strong(expected, desired);
        }

        int expectedState = RotateOp<T>::GRABBED_SECOND,
            desiredState = RotateOp<T>::ROTATED;
        rotateOp.state.compare_exchange_strong(expectedState, desiredState);
      } else if (seen_state == RotateOp<T>::ROTATED) {
        OperationFlaggedPointer expected,
            desired = Singh::flag(op, OperationConstants::NONE);
        parent->op.compare_exchange_strong(
            expected = Singh::flag(op, OperationConstants::ROTATE), desired);
        child->op.compare_exchange_strong(
            expected = Singh::flag(op, OperationConstants::ROTATE), desired);
        Singh::Node<T>* newNode =
            rotateOp.isLeftRotation ? child->left.load() : child->right.load();
        newNode->op.compare_exchange_strong(
            expected = Singh::flag(op, OperationConstants::ROTATE), desired);

        int expectedState = RotateOp<T>::ROTATED,
            desiredState = RotateOp<T>::DONE;
        rotateOp.state.compare_exchange_strong(expectedState, desiredState);
      }
    }
  }

  HeightBalanceState checkBalance(Singh::Node<T>* node, bool forced) {
    if (node->rh - node->lh >= 2 - forced)
      return HeightBalanceState::LEFT_ROTATE;
    else if (node->lh - node->rh >= 2 - forced)
      return HeightBalanceState::RIGHT_ROTATE;
    else
      return HeightBalanceState::NO_ROTATION;
  }

  HeightBalanceState leftRotate(Singh::Node<T>* parent, bool isLeftChild,
                                bool forced) {
    // Assumption: Only called when it is already confirmed to be imbalanced
    if (parent->removed.load())
      return HeightBalanceState::NO_ROTATION;

    Singh::Node<T>*current =
        isLeftChild ? parent->left.load() : parent->right.load(),
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
              parentOp, Singh::flag(rotationOp, OperationConstants::ROTATE))) {
        helpRotate(rotationOp, parent, current, child);
        return HeightBalanceState::LEFT_ROTATE;
      } else {
        delete rotationOp;
        return HeightBalanceState::NO_ROTATION;
      }
    }
    return HeightBalanceState::NO_ROTATION;
  }

  HeightBalanceState rightRotate(Singh::Node<T>* parent, bool isLeftChild,
                                 bool forced) {
    // Assumption: Only called when it is already confirmed to be imbalanced
    if (parent->removed.load())
      return HeightBalanceState::NO_ROTATION;

    Singh::Node<T>*current =
        isLeftChild ? parent->left.load() : parent->right.load(),
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
              parentOp, Singh::flag(rotationOp, OperationConstants::ROTATE))) {
        helpRotate(rotationOp, parent, current, child);
        return HeightBalanceState::RIGHT_ROTATE;
      } else {
        delete rotationOp;
        return HeightBalanceState::NO_ROTATION;
      }
    }
    return HeightBalanceState::NO_ROTATION;
  }

  void cleanup(Singh::Node<T>* node) {
    if (node == nullptr)
      return;
    cleanup(node->left.load());
    cleanup(node->right.load());

    delete node;
  }

  int maintainHelper(Singh::Node<T>* node, Singh::Node<T>* parent,
                     bool isLeftChild, bool forced) {
    if (node == nullptr)
      return 0;
    if (!forced)
      node->lh = maintainHelper(node->left.load(), node, true, false);
    if (!forced)
      node->rh = maintainHelper(node->right.load(), node, false, false);
    node->local_height = std::max(node->lh, node->rh) + 1;

    HeightBalanceState state = checkBalance(node, forced);
    if (state == HeightBalanceState::NO_ROTATION)
      return node->local_height;
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

    if (state != HeightBalanceState::NO_ROTATION)
      node->local_height--;
    return node->local_height;
  }

  void maintain(Singh::Node<T>* root) {
    while (!finished.load()) {
      maintainHelper(root->left.load(), root, true, false);
    }
  }

  void helpMarked(Operation<T>* parentOp, Singh::Node<T>* parent,
                  Singh::Node<T>* node) {
    Singh::Node<T>* child = node->left.load();
    if (child == nullptr)
      node->right.load();

    node->removed = true;
    Operation<T>* casOp = new Operation<T>(std::in_place_type<InsertOp<T>>,
                                           node == parent->left, node, child);
    OperationFlaggedPointer expected =
        reinterpret_cast<OperationFlaggedPointer>(parentOp);
    if (parent->op.compare_exchange_strong(
            expected, Singh::flag(casOp, OperationConstants::INSERT))) {
      helpInsert(casOp, parent);
    }
  }

  void helpInsert(Operation<T>* op, Singh::Node<T>* dest) {
    // TODO: Assumed op to be unflagged
    InsertOp<T>& insertOp = get<InsertOp<T>>(*op);
    if (insertOp.isUpdate) {
      uint8_t expected = 1, desired = 0;
      // Should not encounter 2/3
      dest->deleted.compare_exchange_strong(expected, desired);
    } else {
      std::atomic<Singh::Node<T>*>& addr =
          insertOp.isLeft ? dest->left : dest->right;
      addr.compare_exchange_strong(insertOp.expectedNode, insertOp.newNode);
    }

    OperationFlaggedPointer expected =
                                Singh::flag(op, OperationConstants::INSERT),
                            desired = Singh::flag(op, OperationConstants::NONE);
    dest->op.compare_exchange_strong(expected, desired);
  }

  void help(Singh::Node<T>* parent, OperationFlaggedPointer parentOp,
            Singh::Node<T>* node, OperationFlaggedPointer nodeOp) {
    if (getFlag(nodeOp) == OperationConstants::INSERT) {
      helpInsert(Singh::unFlag<T>(nodeOp), node);
    } else if (getFlag(parentOp) == OperationConstants::ROTATE) {
      Operation<T>* actualOp = Singh::unFlag<T>(parentOp);
      RotateOp<T>& actualRotateOp = get<RotateOp<T>>(*actualOp);
      helpRotate(actualOp, actualRotateOp.parent, actualRotateOp.node,
                 actualRotateOp.child);
    } else if (getFlag(nodeOp) == OperationConstants::MARK) {
      helpMarked(reinterpret_cast<Operation<T>*>(parentOp), parent, node);
    }
  }

  Singh::SeekRecord<T> seek(const T& key) {
    Singh::SeekRecord<T> res{};
    T nodeKey;
    Singh::Node<T>* nxt;

  retry:
    res.result = SeekResultState::NOT_FOUND_L;
    res.node = root;
    res.nodeOp = res.node->op.load();

    if (getFlag(res.nodeOp) == OperationConstants::INSERT) {
      helpInsert(Singh::unFlag<T>(res.nodeOp), res.node);
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
inline Singh::Node<T>* const SinghBBST<T, inf>::sentinel =
    new Singh::Node<T>(T{inf});

template struct SinghBBST<int>;