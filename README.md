This repository contains implementations for various state of the art hashing 
functions ("classic", perfect, minimal perfect, order-preserving).

For further information, see our [collaborative google doc](https://docs.google.com/document/d/1akVt7XBPm3aWRnguZh88jpCAp97yZUwT8V5Po_p2Hxo/edit?usp=sharing)

# Repository layout
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

## Results
Results are stored in the [results](https://github.com/andreaskipf/hashing/tree/main/results) subfolder, together with .pdf plots and python code to generate them.

# Development
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
Currently, this repo contains two separate hashing benchmarks implemented in `collisions.cpp` and `throughput.cpp` respectively. To run them, compile the corresponding targets with cmake (`collision` or `throughput`) and execute the resulting binaries, e.g.:

```bash
$ cmake-build-release/src/collisions \
  -loadfactors=1.0,0.75,0.5,0.25 \
  -datasets=books_200M_uint32.ds:4,books_200M_uint64.ds:8,fb_200M_uint64.ds:8,osm_cellids_200M_uint64.ds:8,wiki_ts_200M_uint64.ds:8 \
  -datapath=data/ \
  -outfile=results/collision.csv
```

* *loadfactors* is a comma separate list of floating point load factors to test. Loadfactors influence the hashtable size, i.e., the amount of slots, for the collision benchmark (`hashtable_size = keyset_size / load_factor`)
* *datasets* is a comma separated list of datasets to perform the benchmark on. Each dataset is denoted in the form `<filename>:<bytes_per_number>`. Currently only 4 or 8 byte unsigned integer value datasets, encoded as `[number_of_entries][first_num]...[last_num]`, where each `[]` denotes a single, little endian encoded unsigned integer value with size `bytes_per_number`. Example datasets may be found [here](https://dataverse.harvard.edu/dataset.xhtml?persistentId=doi:10.7910/DVN/JGVF9A), or, alternatively, synthetically generated using the [data/synthetic.py](https://github.com/andreaskipf/hashing/blob/main/data/synthetic.py) script.
* *datapath* denotes the common prefix path to all datasets. Note that a trailing backslash must currently always be included
* *outfile* denotes the path to the output csv file

Alternatively you can use the [benchmark.sh](https://github.com/andreaskipf/hashing/blob/main/benchmark.sh) script.
