#!/bin/bash

# SET THESE ACCORDING TO YOUR SYSTEM CONFIG
CLANG_BIN=clang
CLANGCPP_BIN=clang++
GCC_BIN=gcc-10
GCCPP_BIN=g++-10

MAX_THREADS=2
LOAD_FACTORS=1.0,0.75,0.5,0.25
SAMPLE_SIZES=0.001,0.01,0.1,1.0
DATASETS="data/books_200M_uint32:4 data/books_200M_uint64:8 data/fb_200M_uint64:8 data/osm_cellids_200M_uint64:8 data/wiki_ts_200M_uint64:8"


# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

# Ensure binaries are freshly built
./build.sh

mkdir -p results/{throughput_hash,throughput_learned,collisions_hash,collisions_learned}

# Clang benchmarks
benchmark/throughput_hash-${CLANGCPP_BIN} \
  --outfile results/throughput_hash/throughput_hash-${CLANGCPP_BIN}.csv \
  --max_threads=${MAX_THREADS} \
  $DATASETS

benchmark/throughput_learned-${CLANGCPP_BIN} \
  --outfile results/throughput_learned/throughput_learned-${CLANGCPP_BIN}.csv \
  --sample_sizes=${SAMPLE_SIZES} \
  --max_threads=${MAX_THREADS} \
  $DATASETS

benchmark/collisions_hash-${CLANGCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --outfile results/collisions_hash/collisions_hash-${CLANGCPP_BIN}.csv \
  --max_threads=${MAX_THREADS} \
  $DATASETS

benchmark/collisions_learned-${CLANGCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --sample_sizes=${SAMPLE_SIZES} \
  --outfile results/collisions_learned/collisions_learned-${CLANGCPP_BIN}.csv \
  --max_threads=${MAX_THREADS} \
  $DATASETS

# GCC benchmarks
benchmark/throughput_hash-${GCCPP_BIN} \
  --outfile results/throughput_hash/throughput_hash-${GCCPP_BIN}.csv \
  --max_threads=${MAX_THREADS} \
  $DATASETS

benchmark/throughput_learned-${GCCPP_BIN} \
  --outfile results/throughput_learned/throughput_learned-${GCCPP_BIN}.csv \
  --sample_sizes=${SAMPLE_SIZES} \
  --max_threads=${MAX_THREADS} \
  $DATASETS

benchmark/collisions_hash-${GCCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --outfile results/collisions_hash/collisions_hash-${GCCPP_BIN}.csv \
  --max_threads=${MAX_THREADS} \
  $DATASETS

benchmark/collisions_learned-${GCCPP_BIN} \
  --load_factors=${LOAD_FACTORS} \
  --sample_sizes=${SAMPLE_SIZES} \
  --outfile results/collisions_learned/collisions_learned-${GCCPP_BIN}.csv \
  --max_threads=${MAX_THREADS} \
  $DATASETS

