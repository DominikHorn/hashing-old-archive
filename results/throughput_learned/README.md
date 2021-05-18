# Throughput for learned hash functions

Results for the throughput experiment using learned hash functions. The experiment (in pseudocode) measures throughput
as follows:

```
raw_sample = sample(dataset)

sample = prepare(raw_sample)

model = Model(sample)

for key in dataset {
    index = reduce(model.predict(key))
}
```

Measures are in place to ensure that the compiler does not eliminate the hash computation but still best effort
optimizes and inlines everything.

Other stats are collected from the experiment but currently not visualized in any plots. See the `.csv` files' header
for more information.

## (g++ 10.2.0) Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz

### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_books_200M_uint32_g++.png)

### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_books_200M_uint64_g++.png)

### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_fb_200M_uint64_g++.png)

### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_osm_cellids_200M_uint64_g++.png)

### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_wiki_ts_200M_uint64_g++.png)

#### consecutive_200M_uint64

All continuous integers from 1000 - 200001000
![consecutive_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_consecutive_200M_uint64_g++.png)

#### gapped_10percent_200M_uint64

All continuous integers, 10% of elements removed starting at 10000
![gapped_10percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_gapped_10percent_200M_uint64_g++.png)

#### gapped_1percent_200M_uint64

All continuous integers, 1% of elements removed starting at 10000
![gapped_1percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_gapped_1percent_200M_uint64_g++.png)

#### gapped_1permill_200M_uint64

All continuous integers, 0.1% of elements removed starting at 10000
![gapped_1permill_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_gapped_1permill_200M_uint64_g++.png)

## (clang++ 11.1.0) Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz

### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_books_200M_uint32_clang++.png)

### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_books_200M_uint64_clang++.png)

### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_fb_200M_uint64_clang++.png)

### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_osm_cellids_200M_uint64_clang++.png)

### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_wiki_ts_200M_uint64_clang++.png)

#### consecutive_200M_uint64

All continuous integers from 1000 - 200001000
![consecutive_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_consecutive_200M_uint64_clang++.png)

#### gapped_10percent_200M_uint64

All continuous integers, 10% of elements removed starting at 10000
![gapped_10percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_gapped_10percent_200M_uint64_clang++.png)

#### gapped_1percent_200M_uint64

All continuous integers, 1% of elements removed starting at 10000
![gapped_1percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_gapped_1percent_200M_uint64_clang++.png)

#### gapped_1permill_200M_uint64

All continuous integers, 0.1% of elements removed starting at 10000
![gapped_1permill_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/throughput_learned/graphs/throughput_learned_gapped_1permill_200M_uint64_clang++.png)
