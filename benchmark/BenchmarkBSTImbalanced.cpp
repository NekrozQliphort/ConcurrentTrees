#include <benchmark/benchmark.h>

#include <vector>
#include "src/CGLBBST/CGLBBST.h"
#include "src/CoarseGrainedLockingBST/CGLBST.h"
#include "src/NatarajanBST/NatarajanBST.h"
#include "src/SinghBBST/SinghBBST.h"

constexpr int SETUP_ELEMS = 32768;
constexpr int TOTAL_ELEMS = 524288;
constexpr int MIN_THREADS = 2;
constexpr int MAX_THREADS = 32;

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
  const int tid = state.thread_index();

  if (tid == 0) {
    for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
         i++) {
      bst.insert(lim - i);
    }
  }

  for (auto _ : state) {
    for (int i = CAPACITY_PER_THREAD * tid, e = CAPACITY_PER_THREAD * (tid + 1);
         i < e; i++) {
      benchmark::DoNotOptimize(bst[i]);
    }
  }
}

static void BM_READ_INTENSIVE_IMBALANCED_SINGLE_THREADED(
    benchmark::State& state) {
  std::set<int> bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS;

  for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
       i++) {
    bst.insert(lim - i);
  }

  for (auto _ : state) {
    for (int i = 0, e = CAPACITY_PER_THREAD; i < e; i++) {
      benchmark::DoNotOptimize(bst.find(i) != bst.end());
    }
  }
}

template <typename BST>
static void BM_READ_WRITE_IMBALANCED(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();
  const int tid = state.thread_index();

  if (tid == 0) {
    for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
         i++) {
      bst.insert(lim - i);
    }

    createBalancedInsertion(elems, 0, CAPACITY_PER_THREAD - 1);
  }

  for (auto _ : state) {
    for (const int elem : elems) {
      const int toBeInserted = TOTAL_ELEMS + CAPACITY_PER_THREAD * tid + elem;
      bst.insert(toBeInserted);
      benchmark::DoNotOptimize(bst[elem]);
      benchmark::DoNotOptimize(bst[toBeInserted]);
      bst.remove(toBeInserted);
    }
  }
}

static void BM_READ_WRITE_IMBALANCED_SINGLE_THREADED(benchmark::State& state) {
  std::set<int> bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS;

  for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
       i++) {
    bst.insert(lim - i);
  }

  createBalancedInsertion(elems, 0, CAPACITY_PER_THREAD - 1);

  for (auto _ : state) {
    for (const int elem : elems) {
      const int toBeInserted = TOTAL_ELEMS + elem;
      bst.insert(toBeInserted);
      benchmark::DoNotOptimize(bst.find(elem) != bst.end());
      benchmark::DoNotOptimize(bst.find(toBeInserted) != bst.end());
      bst.erase(toBeInserted);
    }
  }
}

template <typename BST>
static void BM_WRITE_INTENSIVE_IMBALANCED(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();
  const int tid = state.thread_index();

  if (tid == 0) {
    for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
         i++) {
      bst.insert(lim - i);
    }

    createBalancedInsertion(elems, 0, CAPACITY_PER_THREAD - 1);
  }

  for (auto _ : state) {
    for (const int elem : elems) {
      const int toBeInserted = TOTAL_ELEMS + CAPACITY_PER_THREAD * tid + elem;
      benchmark::DoNotOptimize(bst.insert(toBeInserted));
      benchmark::DoNotOptimize(bst.remove(toBeInserted));
    }
  }
}

static void BM_WRITE_INTENSIVE_IMBALANCED_SINGLE_THREADED(
    benchmark::State& state) {
  std::set<int> bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS;

  for (int i = 0, lim = std::numeric_limits<int>::max() - 3; i < SETUP_ELEMS;
       i++) {
    bst.insert(lim - i);
  }

  createBalancedInsertion(elems, 0, CAPACITY_PER_THREAD - 1);

  for (auto _ : state) {
    for (const int elem : elems) {
      const int toBeInserted = TOTAL_ELEMS + elem;
      bst.insert(toBeInserted);
      bst.erase(toBeInserted);
    }
  }
}

BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<NatarajanBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<CGLBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<CGLBBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED<SinghBBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE_IMBALANCED_SINGLE_THREADED);

BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<NatarajanBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<CGLBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<CGLBBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED<SinghBBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE_IMBALANCED_SINGLE_THREADED);

BENCHMARK(BM_READ_WRITE_IMBALANCED<NatarajanBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED<CGLBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED<CGLBBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED<SinghBBST<int>>)
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE_IMBALANCED_SINGLE_THREADED);

BENCHMARK_MAIN();