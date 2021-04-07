#pragma once

#include <cassert>

#include "../convenience/convenience.hpp"
#include "types.hpp"

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
   static constexpr forceinline HASH_32 mult32_hash(const HASH_32 value,
                                                    const unsigned char p = 32,
                                                    const HASH_32 a = (HASH_32) 0x238EF8E3lu) {
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
   static constexpr forceinline HASH_64 mult64_hash(const HASH_64 value,
                                                    const unsigned char p = 64,
                                                    const HASH_64 a = (HASH_64) 0xC7455FEC83DD661Fllu) {
      assert(p >= 0 && p <= 64);
      return (value * a) >> (64 - p);
   }

   /**
    * mult32_hash with a = floor(((sqrt(5) - 1) / 2) * 2^32)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 32
    */
   static constexpr forceinline HASH_32 fibonacci32_hash(const HASH_32 value, const unsigned char p = 32) {
      return mult32_hash(value, p, 0x9E3779B9lu);
   }

   /**
    * mult64_hash with a = floor(((sqrt(5) - 1) / 2) * 2^64)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 64
    */
   static constexpr forceinline HASH_64 fibonacci64_hash(const HASH_64 value, const unsigned char p = 64) {
      return mult64_hash(value, p, (HASH_64) 0x9E3779B97F4A7C15llu);
   }

   /**
    * mult32_hash with a = closest prime to floor(((sqrt(5) - 1) / 2) * 2^32)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 32
    */
   static constexpr forceinline HASH_32 fibonacci_prime32_hash(const HASH_32 value, const unsigned char p = 32) {
      return mult32_hash(value, p, 0x9e3779b1lu);
   }

   /**
    * mult64_hash with a = closest prime to floor(((sqrt(5) - 1) / 2) * 2^64)
    *
    * @param value the value to hash
    * @param p hash value is \in [0, 2^p]. NOTE: 0 <= p <= 64
    */
   static constexpr forceinline HASH_64 fibonacci_prime64_hash(const HASH_64 value, const unsigned char p = 64) {
      return mult64_hash(value, p, (HASH_64) 0x9E3779B97F4A7C15llu);
   }
};
