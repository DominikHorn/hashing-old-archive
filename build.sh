#!/bin/bash

# SET THESE ACCORDING TO YOUR SYSTEM CONFIG
CLANG_BIN=clang
CLANGCPP_BIN=clang++
GCC_BIN=gcc-10
GCCPP_BIN=g++-10

# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

mkdir -p benchmark

# build with clang
cmake \
  -D CMAKE_BUILD_TYPE=Release \
  -D CMAKE_C_COMPILER=${CLANG_BIN} \
  -D CMAKE_CXX_COMPILER=${CLANGCPP_BIN} \
  -G "CodeBlocks - Unix Makefiles" \
  .
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_hash
mv src/throughput benchmark/throughput_hash-${CLANGCPP_BIN}
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_learned
mv src/throughput benchmark/throughput_learned-${CLANGCPP_BIN}
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_hash
mv src/collisions_hash benchmark/collisions_hash-${CLANGCPP_BIN}
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_learned
mv src/collisions_learned benchmark/collisions_learned-${CLANGCPP_BIN}

# build with gcc
cmake \
  -D CMAKE_BUILD_TYPE=Release \
  -D CMAKE_C_COMPILER=${GCC_BIN} \
  -D CMAKE_CXX_COMPILER=${GCCPP_BIN} \
  -G "CodeBlocks - Unix Makefiles" \
  .
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_hash
mv src/throughput benchmark/throughput_hash-${GCCPP_BIN}
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target throughput_learned
mv src/throughput benchmark/throughput_learned-${GCCPP_BIN}
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_hash
mv src/collisions_hash benchmark/collisions_hash-${GCCPP_BIN}
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release --target collisions_learned
mv src/collisions_learned benchmark/collisions_learned-${GCCPP_BIN}
