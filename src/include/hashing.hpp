#ifndef HASHING_H
#define HASHING_H

#include "convenience.hpp"

/**
 * ----------------------------
 *       Typedefinitions
 * ----------------------------
 */
typedef unsigned long HASH_32;
typedef unsigned long long HASH_64;
typedef struct packed {
   HASH_64 low64;
   HASH_64 high64;
} HASH_128;

/**
 * ----------------------------
 *           xxHash
 * ----------------------------
 */
#include "../../hash_functions/xxHash/xxhash.h"

template<typename T>
HASH_32 forceinline XXH32_hash(T value, XXH32_hash_t seed = 0) {
   return XXH32(&value, sizeof(value), seed);
}

template<typename T>
HASH_64 forceinline XXH64_hash(T value, XXH64_hash_t seed = 0) {
   return XXH64(&value, sizeof(T), seed);
}

template<typename T>
HASH_64 forceinline XXH3_hash(T value) {
   return XXH3_64bits(&value, sizeof(T));
}

template<typename T>
HASH_64 forceinline XXH3_hash_withSeed(T value, XXH64_hash_t seed = 0) {
   return XXH3_64bits_withSeed(&value, sizeof(T), seed);
}

template<typename T>
HASH_128 forceinline XXH3_128_hash(T value) {
   return XXH3_128bits(&value, sizeof(T));
}

template<typename T>
HASH_128 forceinline XXH3_128_hash_withSeed(T value, XXH64_hash_t seed = 0) {
   return XXH3_128bits_withSeed(&value, sizeof(T), seed);
}

// TODO: expose other xxhash functionality, especially batching/streaming

#endif