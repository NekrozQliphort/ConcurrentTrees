#include <benchmark/benchmark.h>

#include <vector>

#include "src/CGLBBST/CGLBBST.h"
#include "src/CoarseGrainedLockingBST/CGLBST.h"
#include "src/FineGrainedLockingBST/FGLBST.h"
#include "src/NatarajanBST/NatarajanBST.h"
#include "src/SinghBBST/SinghBBST.h"

constexpr int TOTAL_ELEMS = 2097152;
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
static void BM_READ_INTENSIVE(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;

  if (state.thread_index() == 0) {
    createBalancedInsertion(elems, 0, TOTAL_ELEMS / state.threads());
    for (const int elem : elems)
      bst.insert(elem);
  }

  for (auto _ : state) {
    long long sum = 0;
    for (const int elem : elems) {
      sum += bst[elem];  // ensure it does not get optimized
    }
    volatile long long temp = sum;
  }
}

template <typename BST>
static void BM_READ_WRITE(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();

  if (state.thread_index() == 0) {
    std::vector<int> initial;
    createBalancedInsertion(initial, 0, TOTAL_ELEMS - 1);
    for (const int initialElem : initial)
      bst.insert(initialElem);

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
static void BM_WRITE_INTENSIVE(benchmark::State& state) {
  BST bst;
  std::vector<int> elems;
  const int CAPACITY_PER_THREAD = TOTAL_ELEMS / state.threads();

  if (state.thread_index() == 0) {
    std::vector<int> initial;
    createBalancedInsertion(initial, 0, TOTAL_ELEMS - 1);
    for (const int initialElem : initial)
      bst.insert(initialElem);

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

BENCHMARK(BM_READ_INTENSIVE<NatarajanBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE<FGLBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE<CGLBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE<CGLBBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_INTENSIVE<SinghBBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);

BENCHMARK(BM_WRITE_INTENSIVE<NatarajanBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE<FGLBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE<CGLBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE<CGLBBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_WRITE_INTENSIVE<SinghBBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);

BENCHMARK(BM_READ_WRITE<NatarajanBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE<FGLBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE<CGLBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE<CGLBBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);
BENCHMARK(BM_READ_WRITE<SinghBBST<int>>)
    ->UseRealTime()
    ->ThreadRange(MIN_THREADS, MAX_THREADS);

BENCHMARK_MAIN();