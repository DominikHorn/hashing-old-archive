#!/bin/bash

# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

# Guarantee clean slate to start
mkdir -p benchmark
find . -name "CMakeCache.txt" -type f | xargs rm

# Build with various compilers. SET THIS ACCORDING TO YOUR SYSTEM CONFIG
for c in clang,clang++ gcc-10,g++-10
do
  _IFS=$IFS
  IFS=","
  set -- $c
  IFS=$_IFS

  cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_C_COMPILER=${1} \
    -D CMAKE_CXX_COMPILER=${2} \
    -G "CodeBlocks - Unix Makefiles" \
    .
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_hash -j
  mv src/throughput_hash benchmark/throughput_hash-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_learned -j
  mv src/throughput_learned benchmark/throughput_learned-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_hash -j
  mv src/collisions_hash benchmark/collisions_hash-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_learned -j
  mv src/collisions_learned benchmark/collisions_learned-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target hashtable_hash -j
  mv src/hashtable_hash benchmark/hashtable_hash-${2}
  cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target hashtable_learned -j
  mv src/hashtable_learned benchmark/hashtable_learned-${2}
done

# Leave clean slate (important for clion interop)
find . -name "CMakeCache.txt" -type f | xargs rm
