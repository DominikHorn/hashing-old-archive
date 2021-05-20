#!/bin/bash

# SET THESE ACCORDING TO YOUR SYSTEM CONFIG
MAX_THREADS=8
LOAD_FACTORS=1.0,0.75,0.5,0.25
SAMPLE_SIZES=0.001,0.01,0.1,1.0
SOSD_DATASETS="data/books_200M_uint32:4 data/books_200M_uint64:8 data/fb_200M_uint64:8 data/osm_cellids_200M_uint64:8 data/wiki_ts_200M_uint64:8"
SYNTH_DATASETS="data/consecutive_200M_uint64:8 data/gapped_1permill_200M_uint64:8 data/gapped_1percent_200M_uint64:8 data/gapped_10percent_200M_uint64:8"
DATASETS="$SOSD_DATASETS $SYNTH_DATASETS"

# Stop on error & cd to script directory
set -e
cd "$(dirname "$0")"

# Ensure binaries are freshly built
./build.sh

# Ensure output directory exists
mkdir -p results/{throughput_hash,throughput_learned,collisions_hash,collisions_learned,hashtable_hash,hashtable_learned}

# Build with various compilers. SET THIS ACCORDING TO YOUR SYSTEM CONFIG
for c in clang,clang++ gcc,g++
do
  _IFS=$IFS
  IFS=","
  set -- $c
  IFS=$_IFS

  benchmark/hashtable_hash-${2} \
    --outfile results/hashtable_hash/hashtable_hash-${2}.csv \
    --max-threads=${MAX_THREADS} \
    $DATASETS

  benchmark/hashtable_learned-${2} \
    --outfile results/hashtable_learned/hashtable_learned-${2}.csv \
    --max-threads=${MAX_THREADS} \
    $DATASETS

  benchmark/collisions_hash-${2} \
    --load-factors=${LOAD_FACTORS} \
    --outfile results/collisions_hash/collisions_hash-${2}.csv \
    --max-threads=${MAX_THREADS} \
    $DATASETS

  benchmark/collisions_learned-${2} \
    --load-factors=${LOAD_FACTORS} \
    --sample-sizes=${SAMPLE_SIZES} \
    --outfile results/collisions_learned/collisions_learned-${2}.csv \
    --max-threads=${MAX_THREADS} \
    $DATASETS

  benchmark/throughput_hash-${2} \
    --outfile results/throughput_hash/throughput_hash-${2}.csv \
    --max-threads=${MAX_THREADS} \
    $DATASETS

  benchmark/throughput_learned-${2} \
    --outfile results/throughput_learned/throughput_learned-${2}.csv \
    --sample-sizes=${SAMPLE_SIZES} \
    --max-threads=${MAX_THREADS} \
    $DATASETS
done
