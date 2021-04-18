This repository contains implementations for various state of the art hashing 
functions ("classic", perfect, minimal perfect, order-preserving).

For further information, see our [collaborative google doc](https://docs.google.com/document/d/1akVt7XBPm3aWRnguZh88jpCAp97yZUwT8V5Po_p2Hxo/edit?usp=sharing)

# Development
## Repository layout
The repository is setup as a monorepo c++ project using CMake. 

* *data/* is meant to contain synthetic and real datasets. You may find a script for generating synthetic example datasets in there aswell. 
  **NOTE: Datasets should never be uploaded to github** and should instead remain on your local device.
* *hash_functions/* contains various hash function implementations wrapped in a single header only library: `hashing.hpp` (to enable full compiler optimizations,   
  like inlining). Some related convenience code/definitions, e.g., `forceinline`, are located in `convenience.hpp`. Hash reduction algorithms, i.e., to reduce 
  values from \[0,2^w\] down to \[0, N\] for arbitrary N, is implemented in `reduction.hpp`
* *libdivide/* contains code for magic number modulo division. This folder will likely be moved into *hash_functions/* in the future
* *results* contains data collected from the benchmarks together with some visualizations
* *src/* contains the actual benchmark targets/implementations. Code shared between benchmarks/potentially useful for multiple benchmarks is located in  
  *src/include* (header only to enable full compiler optimizations, like inlining).

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

# Results
## Throughput
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_books_200M_uint32.ds.png)
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_books_200M_uint64.ds.png)
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_fb_200M_uint64.ds.png)
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_osm_cellids_200M_uint64.ds.png)
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_wiki_ts_200M_uint64.ds.png)

## Collisions
![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/total_colliding_keys_percent_books_200M_uint32.ds.png)
![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/total_colliding_keys_percent_books_200M_uint64.ds.png)
![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/total_colliding_keys_percent_fb_200M_uint64.ds.png)
![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/total_colliding_keys_percent_osm_cellids_200M_uint64.ds.png)
![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/total_colliding_keys_percent_wiki_ts_200M_uint64.ds.png)
