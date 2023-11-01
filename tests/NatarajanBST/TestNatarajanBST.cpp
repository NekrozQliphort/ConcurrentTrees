#include <thread>
#include <vector>

#include "catch.hpp"
#include "src/NatarajanBST/NatarajanBST.h"

TEST_CASE("Insertion - Insertion Race") {
  constexpr int NUM_ITER = 10, NUM_THREADS = 10, NUM_ELEMS_PER_THREAD = 1000;

  for (int i = 0; i < NUM_ITER; i++) {
    NatarajanBST<int> tree;

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