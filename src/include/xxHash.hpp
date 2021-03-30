#ifndef XXHASH_H
#define XXHASH_H

#include "../../opensource_references/xxHash/xxhash.h"
#include "hashing.hpp"
#include "convenience.hpp"

template <typename T>
HASH_32 forceinline XXH32_hash(T value, XXH32_hash_t seed = 0) {
    return XXH32(&value, sizeof(value), seed);
}

template <typename T>
HASH_64 forceinline XXH64_hash(T value, XXH64_hash_t seed = 0) {
    return XXH64(&value, sizeof(T), seed);
}

template <typename T>
HASH_64 forceinline XXH3_hash(T value) {
    return XXH3_64bits(&value, sizeof(T));
}

template <typename T>
HASH_64 forceinline XXH3_hash_withSeed(T value, XXH64_hash_t seed = 0) {
    return XXH3_64bits_withSeed(&value, sizeof(T), seed);
}

template <typename T>
HASH_128 forceinline XXH3_128_hash(T value) {
    return XXH3_128bits(&value, sizeof(T));
}

template <typename T>
HASH_128 forceinline XXH3_128_hash_withSeed(T value, XXH64_hash_t seed = 0) {
    return XXH3_128bits_withSeed(&value, sizeof(T), seed);
}


// TODO: expose other xxhash functionality, especially batching/streaming

#endif