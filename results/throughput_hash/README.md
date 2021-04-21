# Throughput for classical hash functions

Results for the throughput experiment using classical hash functions. The experiment (in pseudocode) measures throughput
as follows:

```
for key in dataset {
    index = reduce(hash(key))
}
```

Measures are in place to ensure that the compiler does not eliminate the hash computation but still best effort
optimizes and inlines everything. All hash methods are fully inlined.

Other stats are collected from the experiment but currently not visualized in any plots. See the `.csv` files' header
for more information.

## g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0 Intel(R) Core(TM) i9-9900K 16 Core @ 3.60GHz

### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-g++_books_200M_uint32.png)

### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-g++_books_200M_uint64.png)

### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-g++_fb_200M_uint64.png)

### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-g++_osm_cellids_200M_uint64.png)

### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-g++_wiki_ts_200M_uint64.png)

## Apple clang version 12.0.0 (clang-1200.0.32.29, macOS 11.2.3, darwin 20.3.0) Intel(R) Core(TM) i5-7360U CPU @ 2.30GHz

### Clang++

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-clang++_books_200M_uint32.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-clang++_books_200M_uint64.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-clang++_fb_200M_uint64.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-clang++_osm_cellids_200M_uint64.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput-clang++_wiki_ts_200M_uint64.png)
