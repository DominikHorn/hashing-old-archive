# Throughput for classical hash functions

Results for the throughput experiment using classical hash functions. The experiment (in pseudocode) measures throughput
as follows:

```
for key in dataset {
    index = reduce(hash(key))
}
```

Measures are in place to ensure that the compiler does not eliminate the hash computation but still best effort
optimizes and inlines everything.

Other stats are collected from the experiment but currently not visualized in any plots. See the `.csv` files' header
for more information.

## g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0 Intel(R) Core(TM) i9-9900K 16 Core @ 3.60GHz

### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_books_200M_uint32_g++.png)

### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_books_200M_uint64_g++.png)

### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_fb_200M_uint64_g++.png)

### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_osm_cellids_200M_uint64_g++.png)

### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_wiki_ts_200M_uint64_g++.png)

#### consecutive_200M_uint64

All continuous integers from 1000 - 200001000
![consecutive_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_consecutive_200M_uint64_g++.png)

#### gapped_10percent_200M_uint64

All continuous integers, 10% of elements removed starting at 10000
![gapped_10percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_gapped_10percent_200M_uint64_g++.png)

#### gapped_1percent_200M_uint64

All continuous integers, 1% of elements removed starting at 10000
![gapped_1percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_gapped_1percent_200M_uint64_g++.png)

#### gapped_1permill_200M_uint64

All continuous integers, 0.1% of elements removed starting at 10000
![gapped_1permill_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash_gapped_1permill_200M_uint64_g++.png)

## Apple clang++ version 12.0.0 (clang-1200.0.32.29, macOS 11.2.3, darwin 20.3.0) Intel(R) Core(TM) i5-7360U CPU @ 2.30GHz

### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash-clang++_books_200M_uint32_g++.png)

### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash-clang++_books_200M_uint64_g++.png)

### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash-clang++_fb_200M_uint64_g++.png)

### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash-clang++_osm_cellids_200M_uint64_g++.png)

### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_hash/graphs/throughput_hash-clang++_wiki_ts_200M_uint64_g++.png)
