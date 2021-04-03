#/bin/bash

# SET THESE ACCORDING TO YOUR SYSTEM CONFIG
CLANG_BIN=clang
CLANGPP_BIN=clang++
GCC_BIN=gcc-10
GCCPP_BIN=g++-10

# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

# build with clang
cmake \
  -D CMAKE_BUILD_TYPE=Release \
  -D CMAKE_C_COMPILER=${CLANG_BIN} \
  -D CMAKE_CXX_COMPILER=${CLANGPP_BIN} \
  -G "CodeBlocks - Unix Makefiles" \
  .
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release
mv src/main benchmark-clang

# build with gcc
cmake \
  -D CMAKE_BUILD_TYPE=Release \
  -D CMAKE_C_COMPILER=${GCC_BIN} \
  -D CMAKE_CXX_COMPILER=${GCCPP_BIN} \
  -G "CodeBlocks - Unix Makefiles" \
  .
cmake --build . --clean-first -DCMAKE_BUILD_TYPE=Release
mv src/main benchmark-gcc
