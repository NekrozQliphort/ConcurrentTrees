#include <thread>
#include <vector>
#include <functional>
#include <iostream>

#include "catch.hpp"
#include "src/FineGrainedLockingBST/FGLBST.h"

TEST_CASE("FGL Insertion sequential check") {
  constexpr int NUM = 1000;
  FGLBST<int> tree;

  for (int i = 0; i < NUM; i++) REQUIRE(!tree[i]);
  for (int i = 0; i < NUM; i++) REQUIRE(tree.insert(i));
  for (int i = 0; i < NUM; i++) REQUIRE(tree[i]);
}

TEST_CASE("FGL Deletion sequential check") {
  SECTION("0/1 child deletion") {
    constexpr int NUM = 1000;
    FGLBST<int> tree;

    for (int i = 0; i < NUM; i++) REQUIRE(tree.insert(i));
    for (int i = 0; i < NUM; i++) {
      REQUIRE(tree.remove(i));
      for (int k = 0; k <= i; k++) REQUIRE(!tree[k]);
      for (int k = i+1; k < NUM; k++) REQUIRE(tree[k]);
    }
  }

  SECTION("2 children deletion") {
    constexpr int NUM = 1000;
    FGLBST<int> tree;

    const std::function<void(int,int)> balancedInsertFunc = [&tree,&balancedInsertFunc](int start, int end) {
      if (start > end) return;
      int mid = start + (end - start) / 2;
      REQUIRE(tree.insert(mid));

      balancedInsertFunc(start, mid-1);
      balancedInsertFunc(mid+1, end);
    };

    balancedInsertFunc(0, NUM-1);

    for (int i = 0; i < NUM; i++) {
      REQUIRE(tree.remove(i));
      for (int k = 0; k <= i; k++) REQUIRE(!tree[k]);
      for (int k = i+1; k < NUM; k++) REQUIRE(tree[k]);
    }
  }
}

TEST_CASE("FGL Insertion - Insertion Race") {
  constexpr int NUM_ITER = 10, NUM_THREADS = 10, NUM_ELEMS_PER_THREAD = 1000;

  for (int i = 0; i < NUM_ITER; i++) {
    FGLBST<int> tree;

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

TEST_CASE("FGL Deletion - Deletion Race") {
  constexpr int NUM_ITER = 10, NUM_THREADS = 50, NUM_ELEMS_PER_THREAD = 400, MOD = 64;

  for (int i = 0; i < NUM_ITER; i++) {
    FGLBST<int> tree;
    // Balanced tree insertion
    const std::function<void(int,int)> balancedInsertFunc = [&tree,&balancedInsertFunc](int start, int end) {
      if (start > end) return;
      int mid = start + (end - start) / 2;
      REQUIRE(tree.insert(mid));

      balancedInsertFunc(start, mid-1);
      balancedInsertFunc(mid+1, end);
    };

    balancedInsertFunc(0, MOD * NUM_ELEMS_PER_THREAD-1);
    for (int num = 0; num < MOD * NUM_ELEMS_PER_THREAD; num++) {
      REQUIRE(tree[num]);
    }

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

    for (int thread = 0; thread < NUM_THREADS; thread++) threads[thread].join();

    for (int num = 0; num < MOD * NUM_ELEMS_PER_THREAD; num++) {
      if (num % MOD < NUM_THREADS) REQUIRE(!tree[num]);
      else REQUIRE(tree[num]);
    }


  }
}