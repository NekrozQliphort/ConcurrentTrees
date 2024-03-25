#include <thread>
#include <vector>

#include "catch.hpp"
#include "src/SinghBBST/SinghBBST.h"
#include "tests/utils.h"

DEFINE_ACCESSOR(SinghBBST<int>, root)
DEFINE_ACCESSOR(SinghBBST<int>, finished)
DEFINE_ACCESSOR(SinghBBST<int>, maintainenceThread)

DEFINE_CALLER(SinghBBST<int>, helpRotate)
DEFINE_CALLER(SinghBBST<int>, maintainHelper)

TEST_CASE("Singh BBST Sanity Check") {
  SinghBBST<int> tree;

  for (int i = 0; i < 1000; i++)
    REQUIRE(!tree[i]);
  for (int i = 0; i < 1000; i++)
    REQUIRE(tree.insert(i));
  for (int i = 0; i < 1000; i++)
    REQUIRE(tree[i]);
}

TEST_CASE("Singh Deletion sequential check") {
  SECTION("0/1 child deletion") {
    constexpr int NUM = 1000;
    SinghBBST<int> tree;

    for (int i = 0; i < NUM; i++)
      REQUIRE(tree.insert(i));
    for (int i = 0; i < NUM; i++) {
      bool check = tree.remove(i);
      REQUIRE(check);
      for (int k = 0; k <= i; k++)
        REQUIRE(!tree[k]);
      for (int k = i + 1; k < NUM; k++)
        REQUIRE(tree[k]);
    }
    done:
    return;
  }

  SECTION("2 children deletion") {
    constexpr int NUM = 1000;
    SinghBBST<int> tree;

    const std::function<void(int, int)> balancedInsertFunc =
        [&tree, &balancedInsertFunc](int start, int end) {
          if (start > end)
            return;
          int mid = start + (end - start) / 2;
          REQUIRE(tree.insert(mid));

          balancedInsertFunc(start, mid - 1);
          balancedInsertFunc(mid + 1, end);
        };

    balancedInsertFunc(0, NUM - 1);

    for (int i = 0; i < NUM; i++) {
      REQUIRE(tree.remove(i));
      for (int k = 0; k <= i; k++)
        REQUIRE(!tree[k]);
      for (int k = i + 1; k < NUM; k++)
        REQUIRE(tree[k]);
    }
  }
}

TEST_CASE("Singh Insertion - Insertion Race") {
  constexpr int NUM_ITER = 100, NUM_THREADS = 10, NUM_ELEMS_PER_THREAD = 1000;

  for (int i = 0; i < NUM_ITER; i++) {
    SinghBBST<int> tree;

    const auto insertFunc = [&tree](int start, int end) {
      for (int k = start; k < end; k++)
        tree.insert(k);
    };

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int thread = 0; thread < NUM_THREADS; thread++) {
      threads.emplace_back(insertFunc, thread * NUM_ELEMS_PER_THREAD,
                           (thread + 1) * NUM_ELEMS_PER_THREAD);
    }

    for (int thread = 0; thread < NUM_THREADS; thread++)
      threads[thread].join();
    for (int num = 0; num < NUM_THREADS * NUM_ELEMS_PER_THREAD; num++)
      REQUIRE(tree[num]);
  }
}

TEST_CASE("Singh Deletion - Deletion Race") {
  constexpr int NUM_ITER = 100, NUM_THREADS = 50, NUM_ELEMS_PER_THREAD = 400,
                MOD = 64;

  for (int i = 0; i < NUM_ITER; i++) {
    SinghBBST<int> tree;
    // Balanced tree insertion
    const std::function<void(int, int)> balancedInsertFunc =
        [&tree, &balancedInsertFunc](int start, int end) {
          if (start > end)
            return;
          int mid = start + (end - start) / 2;
          tree.insert(mid);

          balancedInsertFunc(start, mid - 1);
          balancedInsertFunc(mid + 1, end);
        };

    balancedInsertFunc(0, MOD * NUM_ELEMS_PER_THREAD - 1);

    const auto deleteFunc = [&tree](int start) {
      for (int k = 0; k < NUM_ELEMS_PER_THREAD; k++) {
        int cur = start + MOD * k;
        tree.remove(cur);
      }
    };

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int thread = 0; thread < NUM_THREADS; thread++) {
      threads.emplace_back(deleteFunc, thread);
    }

    for (int thread = 0; thread < NUM_THREADS; thread++)
      threads[thread].join();

    for (int num = 0; num < NUM_THREADS * NUM_ELEMS_PER_THREAD; num++) {
      if (num % MOD < NUM_THREADS)
        REQUIRE(!tree[num]);
      else
        REQUIRE(tree[num]);
    }
  }
}

TEST_CASE("Singh Insertion - Deletion Race") {
  constexpr int NUM_ITER = 100, NUM_THREADS = 64, OFFSET = 16384;
  constexpr int DELETIONS_PER_THREAD = OFFSET / NUM_THREADS;
  constexpr int INSERTIONS_PER_THREAD = 500;

  for (int i = 0; i < NUM_ITER; i++) {
    SinghBBST<int> tree;
    // Balanced tree insertion
    const std::function<void(int, int)> balancedInsertFunc =
        [&tree, &balancedInsertFunc](int start, int end) {
          if (start > end)
            return;
          int mid = start + (end - start) / 2;
          tree.insert(mid);

          balancedInsertFunc(start, mid - 1);
          balancedInsertFunc(mid + 1, end);
        };

    balancedInsertFunc(0, OFFSET - 1);

    const auto deleteFunc = [&tree, &DELETIONS_PER_THREAD](int start) {
      for (int k = start * DELETIONS_PER_THREAD,
               e = (start + 1) * DELETIONS_PER_THREAD;
           k < e; k++) {
        tree.remove(k);
      }
    };

    const auto insertionFunc = [&tree, &OFFSET,
                                &INSERTIONS_PER_THREAD](int start) {
      for (int k = 0; k < INSERTIONS_PER_THREAD; k++) {
        tree.insert(OFFSET + start * INSERTIONS_PER_THREAD + k);
      }
    };

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS * 2);
    for (int thread = 0; thread < NUM_THREADS; thread++) {
      threads.emplace_back(deleteFunc, thread);
      threads.emplace_back(insertionFunc, thread);
    }

    for (int thread = 0; thread < NUM_THREADS * 2; thread++)
      threads[thread].join();

    for (int num = 0; num < OFFSET; num++)
      REQUIRE(!tree[num]);
    for (int num = OFFSET; num < OFFSET + INSERTIONS_PER_THREAD * NUM_THREADS;
         num++)
      REQUIRE(tree[num]);
  }
}

TEST_CASE("Singh helpRotate Sanity Check") {
  SECTION("Left Child Left Rotation") {
    SinghBBST<int> tree;
    PrivateAccess::get_finished(tree).store(true);
    PrivateAccess::get_maintainenceThread(tree).join();

    int toInsert[6]{6, 2, 1, 4, 3, 5};
    for (int i = 0; i < 6; i++)
      tree.insert(toInsert[i]);

    Node<int>* root = PrivateAccess::get_root(tree);
    Node<int>*node1, *node2, *node3, *node4, *node5, *node6;

    node6 = root->left.load();
    node2 = node6->left.load();
    node1 = node2->left.load();
    node4 = node2->right.load();
    node3 = node4->left.load();
    node5 = node4->right.load();

    // Make sure the specific tree structure is correct
    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);

    // Heights dont matter here

    // Create a rotation operation
    Operation<int>* rotationOp = new Operation<int>(
        std::in_place_type<RotateOp<int>>, node6, node2, node4, true, true, SinghBBST<int>::sentinel);
    node3->op.store(flag(rotationOp, OperationConstants::Flags::ROTATE));
    PrivateAccess::call_helpRotate(tree, rotationOp, node6, node2, node4);

    // Check Structure
    node6 = root->left.load();
    node4 = node6->left.load();
    node2 = node4->left.load();
    node1 = node2->left.load();
    node3 = node2->right.load();
    node5 = node4->right.load();

    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);
  }

  SECTION("Right Child Left Rotation") {
    SinghBBST<int> tree;
    PrivateAccess::get_finished(tree).store(true);
    PrivateAccess::get_maintainenceThread(tree).join();
    int toInsert[6]{1, 3, 2, 5, 4, 6};
    for (int i = 0; i < 6; i++)
      tree.insert(toInsert[i]);

    Node<int>* root = PrivateAccess::get_root(tree);
    Node<int>*node1, *node2, *node3, *node4, *node5, *node6;

    node1 = root->left.load();
    node3 = node1->right.load();
    node2 = node3->left.load();
    node5 = node3->right.load();
    node4 = node5->left.load();
    node6 = node5->right.load();

    // Make sure the specific tree structure is correct
    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);

    // Heights dont matter here

    // Create a rotation operation
    Operation<int>* rotationOp = new Operation<int>(
        std::in_place_type<RotateOp<int>>, node1, node3, node5, true, false, SinghBBST<int>::sentinel);
    node3->op.store(flag(rotationOp, OperationConstants::Flags::ROTATE));
    PrivateAccess::call_helpRotate(tree, rotationOp, node1, node3, node5);

    // Check Structure
    node1 = root->left.load();
    node5 = node1->right.load();
    node3 = node5->left.load();
    node6 = node5->right.load();
    node2 = node3->left.load();
    node4 = node3->right.load();

    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);
  }

  SECTION("Left Child Right Rotation") {
    SinghBBST<int> tree;
    PrivateAccess::get_finished(tree).store(true);
    PrivateAccess::get_maintainenceThread(tree).join();

    int toInsert[6]{6, 4, 2, 5, 1, 3};
    for (int i = 0; i < 6; i++)
      tree.insert(toInsert[i]);

    Node<int>* root = PrivateAccess::get_root(tree);
    Node<int>*node1, *node2, *node3, *node4, *node5, *node6;

    node6 = root->left.load();
    node4 = node6->left.load();
    node2 = node4->left.load();
    node1 = node2->left.load();
    node3 = node2->right.load();
    node5 = node4->right.load();

    // Make sure the specific tree structure is correct
    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);

    // Heights dont matter here

    // Create a rotation operation
    Operation<int>* rotationOp = new Operation<int>(
        std::in_place_type<RotateOp<int>>, node6, node4, node2, false, true, SinghBBST<int>::sentinel);
    node3->op.store(flag(rotationOp, OperationConstants::Flags::ROTATE));
    PrivateAccess::call_helpRotate(tree, rotationOp, node6, node4, node2);

    // Check Structure
    node6 = root->left.load();
    node2 = node6->left.load();
    node1 = node2->left.load();
    node4 = node2->right.load();
    node3 = node4->left.load();
    node5 = node4->right.load();

    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);
  }

  SECTION("Right Child Right Rotation") {
    SinghBBST<int> tree;
    PrivateAccess::get_finished(tree).store(true);
    PrivateAccess::get_maintainenceThread(tree).join();
    int toInsert[6]{1, 5, 3, 6, 2, 4};
    for (int i = 0; i < 6; i++)
      tree.insert(toInsert[i]);

    Node<int>* root = PrivateAccess::get_root(tree);
    Node<int>*node1, *node2, *node3, *node4, *node5, *node6;

    node1 = root->left.load();
    node5 = node1->right.load();
    node3 = node5->left.load();
    node6 = node5->right.load();
    node2 = node3->left.load();
    node4 = node3->right.load();

    // Make sure the specific tree structure is correct
    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);

    // Heights dont matter here

    // Create a rotation operation
    Operation<int>* rotationOp = new Operation<int>(
        std::in_place_type<RotateOp<int>>, node1, node5, node3, false, false, SinghBBST<int>::sentinel);
    node5->op.store(flag(rotationOp, OperationConstants::Flags::ROTATE));
    PrivateAccess::call_helpRotate(tree, rotationOp, node1, node5, node3);

    // Check Structure
    node1 = root->left.load();
    node3 = node1->right.load();
    node2 = node3->left.load();
    node5 = node3->right.load();
    node4 = node5->left.load();
    node6 = node5->right.load();

    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
    REQUIRE(node4->key == 4);
    REQUIRE(node5->key == 5);
    REQUIRE(node6->key == 6);
  }
}

TEST_CASE("Double rotation test") {
  SECTION("LR Rotate") {
    SinghBBST<int> tree;
    PrivateAccess::get_finished(tree).store(true);
    PrivateAccess::get_maintainenceThread(tree).join();
    tree.insert(1); tree.insert(3); tree.insert(2);
    Node<int> *root = PrivateAccess::get_root(tree);
    PrivateAccess::call_maintainHelper(tree, root->left.load(), root, true, false);

    Node<int> *node2 = root->left.load();
    Node<int> *node1 = node2->left.load();
    Node<int> *node3 = node2->right.load();
    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
  }

  SECTION("RL Rotate") {
    SinghBBST<int> tree;
    PrivateAccess::get_finished(tree).store(true);
    PrivateAccess::get_maintainenceThread(tree).join();
    tree.insert(3); tree.insert(1); tree.insert(2);
    Node<int> *root = PrivateAccess::get_root(tree);
    PrivateAccess::call_maintainHelper(tree, root->left.load(), root, true, false);

    Node<int> *node2 = root->left.load();
    Node<int> *node1 = node2->left.load();
    Node<int> *node3 = node2->right.load();
    REQUIRE(node1->key == 1);
    REQUIRE(node2->key == 2);
    REQUIRE(node3->key == 3);
  }
}
