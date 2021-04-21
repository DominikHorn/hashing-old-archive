# Collisions Learned

Results for the collisions experiment using learned hash functions. The experiment (in pseudocode) measures collisions
as follows:

```
raw_sample = sample(dataset)

sample = prepare(raw_sample)

model = Model(sample)

for key in dataset {
    index = reduce(model.predict(key))
    collision_counter[index]++
}

colliding_keys = sum([x for x in collision_counter where x > 1])
```

Other stats are collected from the experiment but currently not visualized in any plots. See the `.csv` files' header
for more information.

## Colliding Keys

Amount of colliding keys in percent. Note that these graphs count each key participating in a collision separately,
i.e., a single collision will result in 2 reported colliding keys.

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_books_200M_uint32_g++.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_books_200M_uint64_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/colliding_keys_percent_wiki_ts_200M_uint64_g++.png)

## Sample nanoseconds per key

Amount of time creating the data sample took, relative to the total amount of keys in the keyset. Sampling is done by
repeatedly choosing random elements from the dataset, i.e.:

```
sample = []
for i = 0, i < sample_size, i++ {
    sample[i] = dataset[random_index()]
}
```

Note that for `sample size == 1`, we dont uniformly random sample the dataset but instead memcopy it.

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_books_200M_uint32_g++.png)

#### books_200M_uint64

![books_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_books_200M_uint64_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/sample_nanoseconds_per_key_wiki_ts_200M_uint64_g++.png)

## Sample preparation nanoseconds per key

Amount of time preparing the sample took, relative to the total amount of keys in the keyset. Preparation currently only
consists of sorting the sample.

```
sample = sort(raw_sample)
```

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_books_200M_uint32_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/prepare_nanoseconds_per_key_wiki_ts_200M_uint64_g++.png)

## Build nanoseconds per key

Amount of time building the learned hash function took, relative to the total amount of keys in the keyset. Sampling and
preparation does not count towards build time and is instead listed separately above.

```
model = Model(sample)
```

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_books_200M_uint32_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/build_nanoseconds_per_key_wiki_ts_200M_uint64_g++.png)

## Hashing nanoseconds per key

Amount of time for hashing the entire dataset and recording collisions, relative to the total amount of keys in the
keyset. Collisions are recorded in a `uint32_t` vector with `dataset.size() / load_factor` elements. Since the input
keyset is randomly permutated, each collision_counter increment is expected to result in an L3 cache miss for the read,
thus sort of emulating a hashtable without collision reolution. The following code is measured (in pseudo code form):

```
for key in keyset {
    index = reduce(model.predict(key))
    collision_counter[index]++
}
```

### g++ (Ubuntu 9.3.0-23ubuntu1~16.04) 9.3.0

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_books_200M_uint32_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/hashing_nanoseconds_per_key_wiki_ts_200M_uint64_g++.png)

### Total nanoseconds per key

Total amount of time, i.e., the sum of sample, prepare sample, build and hash, relative to the total amount of keys in
the keyset.

### Clang++

#### books_200M_uint32

![books_200M_uint32](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_books_200M_uint32_g++.png)

#### fb_200M_uint64

![fb_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_fb_200M_uint64_g++.png)

#### osm_cellids_200M_uint64

![osm_cellids_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_osm_cellids_200M_uint64_g++.png)

#### wiki_ts_200M_uint64

![wiki_ts_200M_uint64](https://github.com/andreaskipf/hashing/blob/main/results/collisions_learned/graphs/total_nanoseconds_per_key_wiki_ts_200M_uint64_g++.png)