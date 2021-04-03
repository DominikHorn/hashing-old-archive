#pragma once

#include "../convenience/convenience.hpp"

/**
 * ----------------------------
 *       Typedefinitions
 * ----------------------------
 */
typedef std::uint32_t HASH_32;
typedef std::uint64_t HASH_64;
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
   static constexpr HASH_32 forceinline XXH32_hash(const T& value) {
      return XXH32(value, sizeof(value), 0);
   }

   template<typename T>
   static constexpr HASH_32 forceinline XXH32_hash_withSeed(const T& value, HASH_32 seed) {
      return XXH32(value, sizeof(value), seed);
   }

   template<typename T>
   static constexpr HASH_64 forceinline XXH64_hash(const T& value) {
      return XXH64(&value, sizeof(value), 0);
   }

   template<typename T>
   static constexpr HASH_64 forceinline XXH64_hash_withSeed(const T& value, HASH_64 seed) {
      return XXH64(value, sizeof(value), seed);
   }

   template<typename T>
   static constexpr HASH_64 forceinline XXH3_hash(const T& value) {
      return XXH3_64bits(&value, sizeof(T));
   }

   template<typename T>
   static constexpr HASH_64 forceinline XXH3_hash_withSeed(const T& value, HASH_64 seed) {
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
   /*
    * TODO: for the prime constants, see if we can find better ones (if that is even possible). Especially investigate
    *  0x238EF8E3lu, taken from commit c40d84d in:
    *  https://github.com/peterboncz/bloomfilter-bsd/blob/c3d854c8c6b2fa7788af19c33fa804f80ac4f6cf/src/dtl/hash.hpp#L64
    */

   /*
    * TODO: (?) test performance for random, (probably) non prime constant as suggested in
    *  http://www.vldb.org/pvldb/vol9/p96-richter.pdf
    */

   /**
    * hash = (value * a % 2^32) >> (32 - p)
    * i.e., take the highest p bits from the 32-bit multiplication
    * value * a (ignore overflow)
    *
    * @param value the value to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= 32
    * @param a magic hash constant. You should choose a \in [0,2^32] and a is prime. Defaults to 0x238EF8E3lu
    */
   static constexpr HASH_32 forceinline mult32_hash(const HASH_32& value,
                                                    const unsigned char p = 32,
                                                    const HASH_32& a = (HASH_32) 0x238EF8E3lu) {
      assert(p >= 0 && p <= 32);
      return (value * a) >> (32 - p);
   }

   /**
    * hash = (value * a % 2^64) >> (64 - p)
    * i.e., take the highest p bits from the 64-bit multiplication
    * value * a (ignore overflow)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 64
    * @param a magic hash constant. You should choose a \in [0,2^64] and a is prime. Defaults to 0xC7455FEC83DD661Fllu
    */
   static constexpr HASH_64 forceinline mult64_hash(const HASH_64& value,
                                                    const unsigned char p = 64,
                                                    const HASH_64& a = (HASH_64) 0xC7455FEC83DD661Fllu) {
      assert(p >= 0 && p <= 64);
      return (value * a) >> (64 - p);
   }

   /**
    * mult32_hash with a = floor(((sqrt(5) - 1) / 2) * 2^32)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 32
    */
   static constexpr HASH_32 forceinline fibonacci32_hash(const HASH_32& value, const unsigned char p = 32) {
      return mult32_hash(value, p, 0x9E3779B9lu);
   }

   /**
    * mult64_hash with a = floor(((sqrt(5) - 1) / 2) * 2^64)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 64
    */
   static constexpr HASH_64 forceinline fibonacci64_hash(const HASH_64& value, const unsigned char p = 64) {
      return mult64_hash(value, p, (HASH_64) 0x9E3779B97F4A7C15llu);
   }

   /**
    * mult32_hash with a = closest prime to floor(((sqrt(5) - 1) / 2) * 2^32)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 32
    */
   static constexpr HASH_32 forceinline fibonacci_prime32_hash(const HASH_32& value, const unsigned char p = 32) {
      return mult32_hash(value, p, 0x9e3779b1lu);
   }

   /**
    * mult64_hash with a = closest prime to floor(((sqrt(5) - 1) / 2) * 2^64)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 64
    */
   static constexpr HASH_64 forceinline fibonacci_prime64_hash(const HASH_64& value, const unsigned char p = 64) {
      return mult64_hash(value, p, (HASH_64) 0x9E3779B97F4A7C15llu);
   }

   /**
       * NOTE: 128 bit multiplication is most likely not natively supported on the target CPU, meaning we will
       * spend a lot of extra cpu instructions to emulate this!
       */
   // TODO: this will be useful for mult-add impl
   //      return ((__uint128_t)(value) * (__uint128_t)(a)) >> 64;
};