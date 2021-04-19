#!/bin/bash

# SET THESE ACCORDING TO YOUR SYSTEM CONFIG
CLANG_BIN=clang
CLANGCPP_BIN=clang++
GCC_BIN=gcc-10
GCCPP_BIN=g++-10

LOAD_FACTORS=1.0,0.75,0.5,0.25
SAMPLE_SIZES=0.001,0.01,0.1,1.0
DATASETS="data/books_200M_uint32:4 data/books_200M_uint64:8 data/fb_200M_uint64:8 data/osm_cellids_200M_uint64:8 data/wiki_ts_200M_uint64:8"


# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

# Ensure binaries are freshly built
./build.sh

# Clang benchmarks
benchmark/throughput-${CLANGCPP_BIN} \
  --outfile results/throughput/throughput-${CLANGCPP_BIN}.csv \
  $DATASETS

benchmark/collisions_hash-${CLANGCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --outfile results/collisions_hash/collisions_hash-${CLANGCPP_BIN}.csv \
  $DATASETS

benchmark/collisions_learned-${CLANGCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --sample_sizes=${SAMPLE_SIZES} \
  --outfile results/collisions_learned/collisions_learned-${CLANGCPP_BIN}.csv \
  $DATASETS

# GCC benchmarks
benchmark/throughput-${GCCPP_BIN} \
  --outfile results/throughput/throughput-${GCCPP_BIN}.csv \
  $DATASETS

benchmark/collisions_hash-${GCCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --outfile results/collisions_hash/collisions_hash-${GCCPP_BIN}.csv \
  $DATASETS

benchmark/collisions_learned-${GCCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --sample_sizes=${SAMPLE_SIZES} \
  --outfile results/collisions_learned/collisions_learned-${GCCPP_BIN}.csv \
  $DATASETS

