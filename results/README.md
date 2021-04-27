This folder contains benchmark results for the various experiments in this repository, briefly explained bellow. For
details and visualizations of the obtained data, please see the corresponding subfolders.

Classical hashing and learned hashing results are separated due to pragmatic reasons, i.e., mainly to make maintaining
and updating the benchmark drivers easier.

## Throughput

This experiment aims to measure raw throughput performance for multiple classical and learned hash functions, i.e., is
meant to answer the question "how long does it take to obtain a hash value for each key in the keyset?". The measurement
is performed over a tight loop, allowing for full interleaved execution and similar CPU tricks:

```
begin = time()
for key in keyset {
    hash(key)
}
ns_per_key = (time() - begin) / keyset.size()
```

## Collisions

This experiment aims to measure collision performance of different methods on different datasets, i.e., is meant to
answer the question "what collision characteristics do these methods have for various datasets?":

```
for key in keyset {
    index = hash(key)
    collision_counter[index]++
}

colliding_keys = sum([x for x in collision_counter where x > 1])
```

## Hashtable

This experiment aims to measure 'real world' performance for different hash methods when used in a hashtable, i.e., is
meant to answer the question "How do these hash methods hold up for a real world usecase?". Hashtable insertion, i.e.,
building the hashtable, is measured over a tight loop to get a "best case" baseline:

```
begin = time()
for key in dataset {
    index = hash(key)
    hashtable.insert(index, payload)
}
ns_per_key = (time() - begin) / keyset.size()
```

Hashtable lookup aims to simulate a real usecase, i.e., obtaining and working with the key associated payload.
Therefore, a full memory barrier is inserted at the end of each loop to prevent unwanted and unrealistic interleaved
execution:

```
begin = time()
for key in dataset {
    index = hash(key)
    payload = hashtable.lookup(index, key)
    __sync_synchronize() // gcc full memory fence builtin
}
ns_per_key = (time() - begin) / keyset.size()
```
