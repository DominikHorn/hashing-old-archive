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
