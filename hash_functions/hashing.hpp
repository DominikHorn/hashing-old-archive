#pragma once

#include <cassert>

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
 * Implements different reducers to map values from
 * [0,2^d] into [0, N]
 *
 */
struct HashReduction {
   /**
    * standard modulo reduction. NOTE that modulo is really slow on modern CPU,
    * especially if n is not known at compile time, as a division has to be
    * performed in this case.
    *
    * @tparam should be one of HASH_32 or HASH_64
    * @param value the value to reduce
    * @param n upper bound for result interval
    * @return value mapped to interval [0, n]
    */
   template<typename T>
   static constexpr T forceinline modulo(const T& value, const T& n) {
      return value % n;
   }

   /**
    * Multiply & Shift reduction as proposed by Daniel Lemire:
    * https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    *
    * According to him, expect a 4x performance boost compared to modulo reduction
    * for 32 bit values.
    *
    * NOTE: since this method relies on 2^2w bit arithmetic for w bit values, reducing
    * 64bit values will require 128 bit arithmetic. Afaik this is not hardware supported
    * at the moment.
    *
    * @tparam T should be one of HASH_32 or HASH_64
    * @param value the value to reduce
    * @param n upper bound for the result interval
    * @return value mapped to interval [0, n]
    */
   template<typename T>
   static constexpr T forceinline mult_shift(const T& value, const T& n);

   template<>
   constexpr HASH_32 forceinline mult_shift(const HASH_32& value, const HASH_32& n) {
      return ((uint64_t) value * (uint64_t) n) >> 32;
   }

   template<>
   constexpr HASH_64 forceinline mult_shift(const HASH_64& value, const HASH_64& n) {
      return ((__uint128_t) value * (__uint128_t) n) >> 64;
   }

   /**
    * Reduces value to interval [0, 2^p]
    * @tparam T
    * @param value
    * @param p
    * @return
    */
   template<typename T>
   static constexpr T forceinline shift(const T& value, const unsigned char p = sizeof(T) * 8) {
      return value >> (sizeof(T) * 8 - p);
   }
};

/**
 * ----------------------------
 *           xxHash
 * ----------------------------
 */
#define XXH_INLINE_ALL
#include "xxHash/xxhash.h"

// TODO: also build & benchmark with clang as it supposedly improves performance (at least for xxhash)

struct XXHash {
   // TODO: implement XXHash finalization (probably use a shift operation as with mul?)
   //  i.e., logic to map 32/64 bit results to [0, k], where k is most likely 2^p for some parameter p

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