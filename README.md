This repository aims to benchmark classical vs. learned hash functions. For this
purpose, it contains various state of the art implementations as well as
benchmarking code.

For further information, see our [collaborative google doc](https://docs.google.com/document/d/1akVt7XBPm3aWRnguZh88jpCAp97yZUwT8V5Po_p2Hxo/edit?usp=sharing)

# Table of Contents

- [Table of Contents](#table-of-contents)
- [Development](#development)
  * [Repository layout](#repository-layout)
  * [Setup](#setup)
    + [Cloning this repository](#cloning-this-repository)
    + [Running Benchmarks](#running-benchmarks)
- [Results](#results)
  * [Throughput](#throughput)
    + [Clang++](#clang--)
      - [books_200M_uint32](#books-200m-uint32)
      - [books_200M_uint64](#books-200m-uint64)
      - [fb_200M_uint64](#fb-200m-uint64)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64)
    + [G++-10](#g---10)
      - [books_200M_uint32](#books-200m-uint32-1)
      - [books_200M_uint64](#books-200m-uint64-1)
      - [fb_200M_uint64](#fb-200m-uint64-1)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-1)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-1)
  * [Collisions for classical hash functions](#collisions-for-classical-hash-functions)
    + [Colliding keys](#colliding-keys)
      - [books_200M_uint32](#books-200m-uint32-2)
      - [books_200M_uint64](#books-200m-uint64-2)
      - [fb_200M_uint64](#fb-200m-uint64-2)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-2)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-2)
    + [Nanoseconds per key](#nanoseconds-per-key)
      - [books_200M_uint32](#books-200m-uint32-3)
      - [books_200M_uint64](#books-200m-uint64-3)
      - [fb_200M_uint64](#fb-200m-uint64-3)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-3)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-3)
  * [Collisions for learned hash functions](#collisions-for-learned-hash-functions)
      - [books_200M_uint32](#books-200m-uint32-4)
      - [books_200M_uint64](#books-200m-uint64-4)
      - [fb_200M_uint64](#fb-200m-uint64-4)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-4)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-4)
  * [Sample nanoseconds per key](#sample-nanoseconds-per-key)
    + [Clang++](#clang---1)
      - [books_200M_uint32](#books-200m-uint32-5)
      - [fb_200M_uint64](#fb-200m-uint64-5)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-5)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-5)
    + [G++-10](#g---10-1)
      - [books_200M_uint32](#books-200m-uint32-6)
      - [books_200M_uint64](#books-200m-uint64-5)
      - [fb_200M_uint64](#fb-200m-uint64-6)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-6)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-6)
  * [Prepare sample nanoseconds per key](#prepare-sample-nanoseconds-per-key)
    + [Clang++](#clang---2)
      - [books_200M_uint32](#books-200m-uint32-7)
      - [fb_200M_uint64](#fb-200m-uint64-7)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-7)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-7)
    + [G++-10](#g---10-2)
      - [books_200M_uint32](#books-200m-uint32-8)
      - [books_200M_uint64](#books-200m-uint64-6)
      - [fb_200M_uint64](#fb-200m-uint64-8)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-8)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-8)
  * [Build nanoseconds per key](#build-nanoseconds-per-key)
    + [Clang++](#clang---3)
      - [books_200M_uint32](#books-200m-uint32-9)
      - [fb_200M_uint64](#fb-200m-uint64-9)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-9)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-9)
    + [G++-10](#g---10-3)
      - [books_200M_uint32](#books-200m-uint32-10)
      - [books_200M_uint64](#books-200m-uint64-7)
      - [fb_200M_uint64](#fb-200m-uint64-10)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-10)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-10)
  * [Hashing nanoseconds per key](#hashing-nanoseconds-per-key)
    + [Clang++](#clang---4)
      - [books_200M_uint32](#books-200m-uint32-11)
      - [fb_200M_uint64](#fb-200m-uint64-11)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-11)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-11)
    + [G++-10](#g---10-4)
      - [books_200M_uint32](#books-200m-uint32-12)
      - [books_200M_uint64](#books-200m-uint64-8)
      - [fb_200M_uint64](#fb-200m-uint64-12)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-12)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-12)
  * [Total nanoseconds per key](#total-nanoseconds-per-key)
    + [Clang++](#clang---5)
      - [books_200M_uint32](#books-200m-uint32-13)
      - [fb_200M_uint64](#fb-200m-uint64-13)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-13)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-13)
    + [G++-10](#g---10-5)
      - [books_200M_uint32](#books-200m-uint32-14)
      - [books_200M_uint64](#books-200m-uint64-9)
      - [fb_200M_uint64](#fb-200m-uint64-14)
      - [osm_cellids_200M_uint64](#osm-cellids-200m-uint64-14)
      - [wiki_ts_200M_uint64](#wiki-ts-200m-uint64-14)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>

# Development
## Repository layout
The repository is setup as a monorepo c++ project using CMake. 

* `convenience/` contains an interface library comprised of convenience code 
  (e.g., forceinline macros), used throughout the repository
* `data/` is meant to contain datasets. Also contains a python script for
  generating synthetic datasets and debug/test data. **NOTE: datasets 
  should under no circumstances be uploaded to github (licensing, large file
  size)**. Real world datasets from our results may be found
  [here](https://dataverse.harvard.edu/dataset.xhtml?persistentId=doi:10.7910/DVN/JGVF9A)
* `hashing/` contains an interface library exposing various classical hash
  function implementations, optimized and tuned for small, fixed size keys
* `learned_models/` contains an interface library exposing learned models,
  prepared to be used as a replacement for classical hash functions
* `reduction/` contains an interface library implementing several methods for
  reducing hash values from [0, 2^p] to [0, N]
* `results/` contains benchmark results (csv) as well as plots and python code
  for generating said plots
* `src/` contains the actual benchmarking targets. Each target is implemented
  as a single .cpp file, linking against interface libraries from this
  repository aswell as shared convenience code found in `src/include`
* `thirdparty/` contains an interface library exposing third party libraries
  used by this project, e.g., cxxopts for parsing benchmark cli arguments 

## Setup
### Cloning this repository

Either clone with submodules in one command:
```
git clone --recurse-submodules <repo-url>
```

Or clone regularily and then perform
```
git submodule update --init --recursive
```

### Running Benchmarks
All benchmarks are implemented as single ".cpp" executable targets, located in
src/. To run them, compile the corresponding target with cmake and execute the
resulting binaries. To see the inline help text describing how to work 
with the benchmarks, simply execute the binary without arguments or with "-h" or
with "--help".

Alternatively you may use the `build.sh` or `benchmark.sh` scripts.
The latter will execute `build.sh` automatically.

# Results
## Throughput
### Clang++
#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-clang++_books_200M_uint32.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-clang++_books_200M_uint64.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-clang++_fb_200M_uint64.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-clang++_osm_cellids_200M_uint64.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-clang++_wiki_ts_200M_uint64.png)

### G++-10
#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-g++-10_books_200M_uint32.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-g++-10_books_200M_uint64.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-g++-10_fb_200M_uint64.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-g++-10_osm_cellids_200M_uint64.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput/graphs/throughput-g++-10_wiki_ts_200M_uint64.png)

## Collisions for classical hash functions
### Colliding keys

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_books_200M_uint32_clang++.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_books_200M_uint64_clang++.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_wiki_ts_200M_uint64_clang++.png)

### Nanoseconds per key

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_books_200M_uint32_clang++.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_books_200M_uint64_clang++.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_wiki_ts_200M_uint64_clang++.png)

## Collisions for learned hash functions

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_books_200M_uint32_clang++.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_books_200M_uint64_clang++.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_wiki_ts_200M_uint64_clang++.png)

<!--
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_books_200M_uint32_g++-10.png)
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_books_200M_uint64_g++-10.png)
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_fb_200M_uint64_g++-10.png)
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_osm_cellids_200M_uint64_g++-10.png)
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_wiki_ts_200M_uint64_g++-10.png)
-->

## Sample nanoseconds per key
### Clang++

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_books_200M_uint32_clang++.png)
<!-- ![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_books_200M_uint64_clang++.png) -->

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_wiki_ts_200M_uint64_clang++.png)

### G++-10


#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_books_200M_uint32_g++-10.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_books_200M_uint64_g++-10.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_fb_200M_uint64_g++-10.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_osm_cellids_200M_uint64_g++-10.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_wiki_ts_200M_uint64_g++-10.png)

## Prepare sample nanoseconds per key
### Clang++

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_books_200M_uint32_clang++.png)
<!-- ![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_books_200M_uint64_clang++.png) -->

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_wiki_ts_200M_uint64_clang++.png)

### G++-10

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_books_200M_uint32_g++-10.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_books_200M_uint64_g++-10.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_fb_200M_uint64_g++-10.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_osm_cellids_200M_uint64_g++-10.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_wiki_ts_200M_uint64_g++-10.png)

## Build nanoseconds per key
### Clang++

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_books_200M_uint32_clang++.png)
<!-- ![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_books_200M_uint64_clang++.png)-->

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_wiki_ts_200M_uint64_clang++.png)

### G++-10

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_books_200M_uint32_g++-10.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_books_200M_uint64_g++-10.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_fb_200M_uint64_g++-10.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_osm_cellids_200M_uint64_g++-10.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_wiki_ts_200M_uint64_g++-10.png)

## Hashing nanoseconds per key
### Clang++

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_books_200M_uint32_clang++.png)
<!-- ![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_books_200M_uint64_clang++.png) -->

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_wiki_ts_200M_uint64_clang++.png)

### G++-10

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_books_200M_uint32_g++-10.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_books_200M_uint64_g++-10.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_fb_200M_uint64_g++-10.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_osm_cellids_200M_uint64_g++-10.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_wiki_ts_200M_uint64_g++-10.png)

## Total nanoseconds per key
### Clang++

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_books_200M_uint32_clang++.png)
<!-- ![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_books_200M_uint64_clang++.png) -->

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_fb_200M_uint64_clang++.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_osm_cellids_200M_uint64_clang++.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_wiki_ts_200M_uint64_clang++.png)

### G++-10

#### books_200M_uint32
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_books_200M_uint32_g++-10.png)

#### books_200M_uint64
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_books_200M_uint64_g++-10.png)

#### fb_200M_uint64
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_fb_200M_uint64_g++-10.png)

#### osm_cellids_200M_uint64
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_osm_cellids_200M_uint64_g++-10.png)

#### wiki_ts_200M_uint64
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_wiki_ts_200M_uint64_g++-10.png)
