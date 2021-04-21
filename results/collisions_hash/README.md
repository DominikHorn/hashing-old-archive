# Collisions for classical hash functions

Results for the collisions experiment using classical hash functions. The experiment (in pseudocode) measures collisions
as follows:

```
for key in dataset {
    index = reduce(hash(key))
    collision_counter[index]++
}

colliding_keys = sum([x for x in collision_counter where x > 1])
```

Other stats are collected from the experiment but currently not visualized in any plots. See the `.csv` files' header
for more information.

## Colliding keys

Amount of colliding keys in percent. Note that these graphs count each key participating in a collision separately,
i.e., a single collision will result in 2 reported colliding keys.

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0 Intel(R) Core(TM) i9-9900K 16 Core @ 3.60GHz

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_books_200M_uint32_g++.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_books_200M_uint64_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/colliding_keys_percent_wiki_ts_200M_uint64_g++.png)

## Nanoseconds per key

Total amount of time for hashing the entire dataset and measuring collisions relative to the amount of keys in the
keyset. Since these hashfunctions aim to produce randomly distributed uniform hash values, each collision_counter
increment is expected to result in an L3 cache miss for the read, thus sort of emulating a hashtable without collision
resolution. The following code is measured (in pseudo code form):

```
for key in keyset {
    index = reduce(hash(key))
    collision_counter[index]++
}
```

**NOTE:** For some reason Clang is able to produce significantly more efficient MeowHash machine code. Sample benchmarks
indicate up to 80x performance gain when compiled with Clang, making MeowHash the fasted hashing schema throughout.
Proper benchmarking will be conducted with clang in the future.

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0 Intel(R) Core(TM) i9-9900K 16 Core @ 3.60GHz

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_books_200M_uint32_g++.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_books_200M_uint64_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_hash/graphs/nanoseconds_per_key_wiki_ts_200M_uint64_g++.png)
