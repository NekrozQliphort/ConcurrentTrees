#include <benchmark/benchmark.h>

#include <vector>
#include "src/CGLBBST/CGLBBST.h"
#include "src/CoarseGrainedLockingBST/CGLBST.h"
#include "src/NatarajanBST/NatarajanBST.h"
#include "src/SinghBBST/SinghBBST.h"

constexpr int SETUP_ELEMS = 32768;
constexpr int TOTAL_ELEMS = 524288;
constexpr int MIN_THREADS = 2;
constexpr int MAX_THREADS = 64;

void createBalancedInsertion(std::vector<int>& container, int start, int end) {
  if (start > end)
    return;
  int mid = start + (end - start) / 2;
  container.push_back(mid);
  createBalancedInsertion(container, start, mid - 1);
  createBalancedInsertion(container, mid + 1, end);
}

template <typename BST>
static void BM_READ_INTENSIVE_IMBALANCED(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();

  if (state.thread_index() == 0) {
    for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
         i++) {
      bst.insert(lim - i);
    }
  }

  for (auto _ : state) {
    long long sum = 0;
    const int tid = state.thread_index();
    for (int i = CAPACITY_PER_THREAD * tid,
             e = state.thread_index() * (tid + 1);
         i < e; i++) {
      sum += bst[i];  // ensure it does not get optimized
    }
    volatile long long temp = sum;
  }
}

template <typename BST>
static void BM_READ_WRITE_IMBALANCED(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();

  if (state.thread_index() == 0) {
    for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
         i++) {
      bst.insert(lim - i);
    }

    createBalancedInsertion(elems, 0, CAPACITY_PER_THREAD - 1);
  }

  for (auto _ : state) {
    long long sum = 0;
    for (const int elem : elems) {
      const int toBeInserted =
          TOTAL_ELEMS + CAPACITY_PER_THREAD * state.thread_index() + elem;
      bst.insert(toBeInserted);
      sum += bst[elem];
      sum += bst[toBeInserted];
      bst.remove(toBeInserted);
    }
    volatile long long temp = sum;
  }
}

template <typename BST>
static void BM_WRITE_INTENSIVE_IMBALANCED(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();

  if (state.thread_index() == 0) {
    for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
         i++) {
      bst.insert(lim - i);
    }

    createBalancedInsertion(elems, 0, CAPACITY_PER_THREAD - 1);
  }

  for (auto _ : state) {
    long long sum = 0;
    for (const int elem : elems) {
      const int toBeInserted =
          TOTAL_ELEMS + CAPACITY_PER_THREAD * state.thread_index() + elem;
      sum += bst.insert(toBeInserted);
      sum += bst.remove(toBeInserted);
    }
    volatile long long temp = sum;
  }
}

BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<NatarajanBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<CGLBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<CGLBBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<SinghBBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);

BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<NatarajanBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<CGLBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<CGLBBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<SinghBBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);

BENCHMARK(BM_READ_WRITE_IMBALANCED<NatarajanBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED<CGLBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED<CGLBBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED<SinghBBST<int>>)
//    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);

BENCHMARK_MAIN();