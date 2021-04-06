#pragma once

#include <cassert>

#include "../convenience/convenience.hpp"

/**
 * ----------------------------
 *      General & Typedefs
 * ----------------------------
 */
#define HASH_32 std::uint32_t
#define HASH_64 std::uint64_t
struct HASH_128 {
   HASH_64 lower;
   HASH_64 higher;
};

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

   /**
    * Takes the lower 64 bits of a 128 bit hash value
    * @param value
    * @return
    */
   static constexpr HASH_64 forceinline lower_half(const HASH_128& value) {
      return value.lower;
   }

   /**
    * Takes the upper 64 bits of a 128 bit hash value
    * @param value
    * @return
    */
   static constexpr HASH_64 forceinline upper_half(const HASH_128& value) {
      return value.higher;
   }

   /**
    * xors the upper and lower 64-bits of a 128 bit value to obtain a final 64 bit hash
    * @param value
    * @return
    */
   static constexpr HASH_64 forceinline xor_both(const HASH_128& value) {
      return value.higher ^ value.lower;
   }
};

template<>
constexpr HASH_32 forceinline HashReduction::mult_shift(const HASH_32& value, const HASH_32& n) {
   return ((uint64_t) value * (uint64_t) n) >> 32;
}

template<>
constexpr HASH_64 forceinline HashReduction::mult_shift(const HASH_64& value, const HASH_64& n) {
   return ((__uint128_t) value * (__uint128_t) n) >> 64;
}

/**
 * ----------------------------
 *           xxHash
 * ----------------------------
 */
#define XXH_INLINE_ALL
#include "xxHash/xxhash.h"

// TODO: benchmark with clang & gcc builds as clang supposedly improves xxHash performance
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
    * TODO: (?) test performance for random, (likely) non prime constant as suggested in
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
    * @param a magic hash constant. You should choose a \in [0,2^64] and a is prime. Defaults to 0xC7455FEC83DD661F
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

/**
 * ----------------------------
 *      MultAdd Hashing
 * ----------------------------
 */
struct MultAddHash {
   /**
    * hash = ((value * a + b) % 2^64) >> (64 - p)
    *
    * @param value the value to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= 32
    * @param a magic hash constant. You should choose a \in [0,2^64] and a is prime. Defaults to 0xC16FD7EEC0213493
    * @param b magic hash constant. You should choose b \in [0,2^64] and b is prime. Defaults to 0xA501000042C9A6E3
    */
   static constexpr HASH_32 forceinline multadd32_hash(const HASH_32& value,
                                                       const unsigned char p = 32,
                                                       const HASH_64& a = (HASH_64) 0xC16FD7EEC0213493llu,
                                                       const HASH_64& b = (HASH_64) 0xA501000042C9A6E3llu) {
      assert(p >= 0 && p <= 32);
      return ((uint64_t) value * a + b) >> (64 - p);
   }

   /**
    * hash = ((value * a + b) % 2^128) >> (128 - p)
    *
    * @param value the value to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= 64
    * @param a magic hash constant. You should choose a \in [0,2^128] and a is prime. Defaults to 0x9B6E895FDDB88E3109E77036171A861D
    * @param b magic hash constant. You should choose b \in [0,2^128] and b is prime. Defaults to 0xF89E2E1E25B514732113E4015584C8AF
    */
   static constexpr HASH_64 forceinline multadd64_hash(const HASH_64& value,
                                                       const unsigned char p = 64,
                                                       const HASH_64& a_low = (HASH_64) 0x09E77036171A861Dllu,
                                                       const HASH_64& a_high = (HASH_64) 0x9B6E895FDDB88E31llu,
                                                       const HASH_64& b_low = (HASH_64) 0x2113E4015584C8AFllu,
                                                       const HASH_64& b_high = (HASH_64) 0xF89E2E1E25B51473llu) {
      assert(p >= 0 && p <= 64);
      const __uint128_t a = (__uint128_t) a_low + ((__uint128_t) a_high << 64);
      const __uint128_t b = (__uint128_t) b_low + ((__uint128_t) b_high << 64);
      return ((__uint128_t) value * a + b) >> (128 - p);
   }
};

/**
 * ----------------------------
 *      Murmur Hashing
 * ----------------------------
 *
 * Implementations taken from Austin Appleby's original code:
 * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530),
 * or the submodule hash_functions/murmur/src/MurmurHash3.cpp
 *
 * Code had to be copied from .cpp file to facilitate inlining.
 * This is afaik okay since Austin Appleby, the author, disclaimed
 * any copyright (see header comment in MurmurHash3.cpp)
 */
struct MurmurHash3 {
   /**
    * Murmur 32-bit finalizer
    *
    * Richter, Stefan, Victor Alvarez, and Jens Dittrich "A seven-dimensional
    * analysis of hashing methods and its implications on query processing."
    * (PVLDB 9.3 (2015): 96-107.) only use the finalizer, meaning they skip
    * some computations, which might be important for hash quality. Therefore:
    * TODO: investigate impact on quality when using this shortcut
    *
    * @param value the value to finalize
    * @return 32 bit hash
    */
   static constexpr HASH_32 forceinline finalize_32(HASH_32 value) {
      value ^= value >> 16;
      value *= 0x85ebca6blu;
      value ^= value >> 13;
      value *= 0xc2b2ae35lu;
      value ^= value >> 16;

      return value;
   }

   /**
    * Murmur 64-bit finalizer
    *
    * Richter, Stefan, Victor Alvarez, and Jens Dittrich "A seven-dimensional
    * analysis of hashing methods and its implications on query processing."
    * (PVLDB 9.3 (2015): 96-107.) only use the finalizer, meaning they skip
    * some computations, which might be important for hash quality. Therefore:
    * TODO: investigate impact on quality when using this shortcut
    *
    * @param value the value to finalize
    * @return 64 bit hash
    */
   static constexpr HASH_64 forceinline finalize_64(HASH_64 value) {
      value ^= value >> 33;
      value *= 0xff51afd7ed558ccdllu;
      value ^= value >> 33;
      value *= 0xc4ceb9fe1a85ec53llu;
      value ^= value >> 33;

      return value;
   }

   /**
    * Murmur3 32-bit, adjusted to fixed 32-bit input values (compiler would presumably perform the same optimizations.
    * However, in this explicit form it is clear what computation actually happens. This might be important for the
    * argument "VLDB paper misrepresents murmur quality, here's what murmur actually computes:").
    *
    * @param value
    * @param seed murmur seed, defaults to 0x238EF8E3 (random 32-bit prime)
    * @return
    */
   static constexpr HASH_32 murmur3_32(const HASH_32& value, const HASH_32& seed = 0x238EF8E3lu) {
      const auto len = sizeof(HASH_32);
      // nblocks = len / 4 = sizeof(value) / 4  = 4 / 4 = 1

      uint32_t h1 = seed;

      uint32_t c1 = 0xcc9e2d51;
      uint32_t c2 = 0x1b873593;

      //----------
      // body

      // getblock(blocks, -1) = getblock((void*)(&value) + 1 * 4, -1) = getblock(&value + 1, -1) = (&value + 1)[-1] = *(&value + 1 - 1) = value
      uint32_t k1 = value;

      k1 *= c1;
      k1 = rotl32(k1, 15);
      k1 *= c2;

      h1 ^= k1;
      h1 = rotl32(h1, 13);
      h1 = h1 * 5 * 0xe6546b64;

      //----------
      // tail

      // len & 3 = sizeof(value) & 3 = 4 & 3 = 0, therefore nothing happens for tail computation

      //----------
      // finalization

      h1 ^= len;
      h1 = finalize_32(h1);
      return h1;
   }

   /**
    * Murmur3 128-bit, adjusted to fixed 64-bit input values (compiler would presumably perform the same optimizations.
    * However, in this explicit form it is clear what computation actually happens. This might be important for the
    * argument "VLDB paper misrepresents murmur quality, here's what murmur actually computes:").
    *
    * @param value
    * @param seed murmur seed, defaults to 0xC7455FEC83DD661F (random 64-bit prime)
    * @return
    */
   static constexpr HASH_128 murmur3_128(const HASH_64& value, const HASH_64& seed = (HASH_64) 0xC7455FEC83DD661Fllu) {
      // nblocks = len / 16 = sizeof(value) / 16  = 8 / 16 = 0 (int division)

      uint64_t h1 = seed;
      uint64_t h2 = seed;

      uint64_t c1 = 0x87c37b91114253d5llu;
      uint64_t c2 = 0x4cf5ad432745937fllu;

      //----------
      // body

      // since nblocks = 0, loop will never execute

      //----------
      // tail

      // nblocks = 0, data just points at value
      const uint8_t* tail = (const uint8_t*) (&value);

      uint64_t k1 = 0;
      uint64_t k2 = 0;

      // len = sizeof(value) = 8
      k1 ^= uint64_t(tail[7]) << 56;
      k1 ^= uint64_t(tail[6]) << 48;
      k1 ^= uint64_t(tail[5]) << 40;
      k1 ^= uint64_t(tail[4]) << 32;
      k1 ^= uint64_t(tail[3]) << 24;
      k1 ^= uint64_t(tail[2]) << 16;
      k1 ^= uint64_t(tail[1]) << 8;
      k1 ^= uint64_t(tail[0]) << 0;
      k1 *= c1;
      k1 = rotl64(k1, 31);
      k1 *= c2;
      h1 ^= k1;

      //----------
      // finalization

      h1 ^= sizeof(HASH_64);
      h2 ^= sizeof(HASH_64);

      h1 += h2;
      h2 += h1;

      h1 = finalize_64(h1);
      h2 = finalize_64(h2);

      h1 += h2;
      h2 += h1;

      return {.lower = h2, .higher = h1};
   }

  private:
   /// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530)
   static constexpr uint32_t forceinline rotl32(uint32_t x, int8_t r) {
      return (x << r) | (x >> (32 - r));
   }

   /// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530)
   static constexpr uint64_t forceinline rotl64(uint64_t x, int8_t r) {
      return (x << r) | (x >> (64 - r));
   }
};