#pragma once

#include <cstdint>

#include "convenience.hpp"

/**
 * ----------------------------
 *       Typedefinitions
 * ----------------------------
 */
typedef uint32_t HASH_32;
typedef uint64_t HASH_64;
typedef struct packed {
   HASH_64 low64;
   HASH_64 high64;
} HASH_128;

/**
 * ----------------------------
 *           xxHash
 * ----------------------------
 */
#define XXH_INLINE_ALL
#include "xxhash.h"

struct XXHash {
   template<typename T>
   static HASH_32 forceinline XXH32_hash(T value) {
      return XXH32(&value, sizeof(value), 0);
   }

   template<typename T>
   static HASH_32 forceinline XXH32_hash_withSeed(T value, HASH_32 seed) {
      return XXH32(&value, sizeof(value), seed);
   }

   template<typename T>
   static HASH_64 forceinline XXH64_hash(T value) {
      return XXH64(&value, sizeof(value), 0);
   }

   template<typename T>
   static HASH_64 forceinline XXH64_hash_withSeed(T value, HASH_64 seed) {
      return XXH64(&value, sizeof(value), seed);
   }

   template<typename T>
   static HASH_64 forceinline XXH3_hash(T value) {
      return XXH3_64bits(&value, sizeof(T));
   }

   template<typename T>
   static HASH_64 forceinline XXH3_hash_withSeed(T value, HASH_64 seed) {
      return XXH3_64bits_withSeed(&value, sizeof(T), seed);
   }

   // TODO: do we need 128 bit hashing?

   //   template<typename T>
   //   static HASH_128 forceinline XXH3_128_hash(T value) {
   //      return XXH3_128bits(&value, sizeof(T));
   //   }
   //
   //   template<typename T>
   //   static HASH_128 forceinline XXH3_128_hash_withSeed(T value, HASH_64 seed) {
   //      return XXH3_128bits_withSeed(&value, sizeof(T), seed);
   //   }

   // TODO: expose other xxhash functionality, especially batching/streaming
};

