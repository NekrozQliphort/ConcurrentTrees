#!/bin/bash
#
#SBATCH --job-name=benchmarkBST
#SBATCH --output=benchmark/benchmark_results.out
#SBATCH --time=1:00:00
#SBATCH --partition="medium"
#SBATCH --constraint="xcnf"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all -j 6
export BENCHMARK_BUILD="build/BenchmarkBSTImbalanced"
./${BENCHMARK_BUILD}