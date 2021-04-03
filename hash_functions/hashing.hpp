#pragma once

#include <cstdint>

#include "../convenience/convenience.hpp"

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
#include "xxHash/xxhash.h"

struct XXHash {
   template<typename T>
   static HASH_32 forceinline XXH32_hash(const T& value) {
      return XXH32(value, sizeof(value), 0);
   }

   template<typename T>
   static HASH_32 forceinline XXH32_hash_withSeed(const T& value, HASH_32 seed) {
      return XXH32(value, sizeof(value), seed);
   }

   template<typename T>
   static HASH_64 forceinline XXH64_hash(const T& value) {
      return XXH64(&value, sizeof(value), 0);
   }

   template<typename T>
   static HASH_64 forceinline XXH64_hash_withSeed(const T& value, HASH_64 seed) {
      return XXH64(value, sizeof(value), seed);
   }

   template<typename T>
   static HASH_64 forceinline XXH3_hash(const T& value) {
      return XXH3_64bits(&value, sizeof(T));
   }

   template<typename T>
   static HASH_64 forceinline XXH3_hash_withSeed(const T& value, HASH_64 seed) {
      return XXH3_64bits_withSeed(value, sizeof(T), seed);
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

   // TODO: expose other xxhash functionality, especially streaming (?)
};

/**
 * ----------------------------
 *    Multiplicative Hashing
 * ----------------------------
 */
struct MultHash {
   static HASH_32 forceinline mult32_hash(const HASH_32& value) {
      // TODO: this is probably a random prime number, taken from:
      //  https://github.com/peterboncz/bloomfilter-bsd/blob/c3d854c8c6b2fa7788af19c33fa804f80ac4f6cf/src/dtl/hash.hpp#L64
      //  (commit c40d84d)
      //  Investigate & see if we can find a better one (should not really matter?)/test with different primes?
      return value * 0x238EF8E3lu;
   }

   static HASH_64 forceinline mult64_hash(const HASH_64& value) {
      // TODO: this is a randomly generated prime constant, see if we can find a better one? (should not really matter?)
      return value * 0xC7455FEC83DD661Fllu;
   }

   static HASH_32 forceinline fibonacci32_hash(const HASH_32& value) {
      // floor(((sqrt(5) - 1) / 2) * 2^32)
      return value * 0x9E3779B9lu;
   }

   static HASH_32 forceinline fibonacci_prime32_hash(const HASH_32& value) {
      // Closest prime to the 32bit fibonacci hash constant (see above)
      return value * 0x9e3779b1lu;
   }

   static HASH_64 forceinline fibonacci64_hash(const HASH_64& value) {
      // floor(((sqrt(5) - 1) / 2) * 2^64)
      return value * 0x9E3779B97F4A7C15llu;
   }

   static HASH_64 forceinline fibonacci_prime64_hash(const HASH_64& value) {
      // Closest prime to the 64bit fibonacci hash constant (see above)
      return value * 0x9E3779B97F4A7C55llu;
   }
};