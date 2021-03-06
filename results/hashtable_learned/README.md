# Hashtable Learned

Results for the hashtable experiment using learned hash functions. Currently, the only tested hashtable implementation
is a chained hashtable with varying bucket sizes. The experiment (in pseudocode) measures insert and lookup performance
as follows:

```
// Insert in tight loop
for key in dataset {
    index = reduce(model.predict(key))
    hashtable.insert(index, payload)
}

// Lookup. Simulate working with payload through full memory fence
for key in dataset {
    index = reduce(model.predict(key))
    payload = hashtable.lookup(index, key)
    __sync_synchronize() // gcc full memory fence builtin
}
```

**NOTE:** the chained hashtable implementation uses `__attribute__((packed))` buckets, which causes missaligned reads
and writes. This was done to save a significant amount of main memory, i.e., to be able to run some experiments which
would have otherwise thrown `std::bad_alloc` due to main memory constraints. As a matter of fact, results for load
factor 0.25, bucket size 4 are missing since they could not be obtained even with this optimization.

Experimenting with configurations that run even when buckets use padding revealed that there is only a very small
difference for packed vs unpacked buckets in terms of both lookup and insert performance. Additionally, missaligned
reads/writes should not favor any of the tested methods and will therefore not influence the meaningfullness of these
results.

## Insert

Time to insert each key in a tight loop:

```
for key in dataset {
    index = reduce(model.predict(key))
    hashtable.insert(index, payload)
}
```

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0 Intel(R) Core(TM) i9-9900K 16 Core @ 3.60GHz

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_books_200M_uint32_g++.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_books_200M_uint64_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_wiki_ts_200M_uint64_g++.png)

#### consecutive_200M_uint64

All continuous integers from 1000 - 200001000
![consecutive_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_consecutive_200M_uint64_g++.png)

#### gapped_10percent_200M_uint64

All continuous integers, 10% of elements removed starting at 10000
![gapped_10percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_gapped_10percent_200M_uint64_g++.png)

#### gapped_1percent_200M_uint64

All continuous integers, 1% of elements removed starting at 10000
![gapped_1percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_gapped_1percent_200M_uint64_g++.png)

#### gapped_1permill_200M_uint64

All continuous integers, 0.1% of elements removed starting at 10000
![gapped_1permill_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/insert_gapped_1permill_200M_uint64_g++.png)

## Lookup

Time to lookup each payload, simulating working with the payload by using a memory fence:

```
for key in dataset {
    index = reduce(model.predict(key))
    payload = hashtable.lookup(index, key)
    __sync_synchronize() // gcc full memory fence builtin
}
```

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0 Intel(R) Core(TM) i9-9900K 16 Core @ 3.60GHz

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_books_200M_uint32_g++.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_books_200M_uint64_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_wiki_ts_200M_uint64_g++.png)

#### consecutive_200M_uint64

All continuous integers from 1000 - 200001000
![consecutive_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_consecutive_200M_uint64_g++.png)

#### gapped_10percent_200M_uint64

All continuous integers, 10% of elements removed starting at 10000
![gapped_10percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_gapped_10percent_200M_uint64_g++.png)

#### gapped_1percent_200M_uint64

All continuous integers, 1% of elements removed starting at 10000
![gapped_1percent_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_gapped_1percent_200M_uint64_g++.png)

#### gapped_1permill_200M_uint64

All continuous integers, 0.1% of elements removed starting at 10000
![gapped_1permill_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/hashtable_learned/graphs/lookup_gapped_1permill_200M_uint64_g++.png)
