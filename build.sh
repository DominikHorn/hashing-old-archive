#!/bin/bash

# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

mkdir -p benchmark

# Build with various compilers. SET THIS ACCORDING TO YOUR SYSTEM CONFIG
compilers=(clang++ g++-10)
for c in clang,clang++ gcc-10,g++-10
do
  IFS=","
  set -- $c

  cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_C_COMPILER=${1} \
    -D CMAKE_CXX_COMPILER=${2} \
    -G "CodeBlocks - Unix Makefiles" \
    .
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_hash
  mv src/throughput_hash benchmark/throughput_hash-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_learned
  mv src/throughput_learned benchmark/throughput_learned-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_hash
  mv src/collisions_hash benchmark/collisions_hash-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_learned
  mv src/collisions_learned benchmark/collisions_learned-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target hashtable_hash
  mv src/hashtable_hash benchmark/hashtable_hash-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target hashtable_learned
  mv src/hashtable_learned benchmark/hashtable_learned-${2}
done