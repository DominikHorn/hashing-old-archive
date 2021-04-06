#pragma once

#include "../convenience/convenience.hpp"
#include "types.hpp"

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
