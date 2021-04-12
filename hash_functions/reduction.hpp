#pragma once

#include "convenience.hpp"
#include "wrappers/types.hpp"

// TODO: we might need those
//#define LIBDIVIDE_SSE2
//#define LIBDIVIDE_AVX2
//#define LIBDIVIDE_AVX512
//#define LIBDIVIDE_NEON
#include "../libdivide/libdivide.h"

/**
 * Implements different reducers to map values from
 * [0,2^d] into [0, N]
 *
 */
struct HashReduction {
   /**
    * NOOP reduction, i.e., doesn't do anything
    */
   template<typename T>
   static constexpr forceinline T do_nothing(const T& value, const T& n) {
      return value;
   }

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
   static constexpr forceinline T modulo(const T& value, const T& n) {
      return value % n;
   }

   template<typename T>
   static constexpr forceinline T magic_modulo(const T& value, const T& n, const libdivide::divider<T>& fast_d) {
      const auto div = value / fast_d; // Operator overloading ensures this is not an actual division
      const auto remainder = value - div * n;
      return remainder;
   }

   template<typename T>
   static forceinline libdivide::divider<T> make_magic_divider(const T& divisor) {
      // TODO: similar to https://github.com/peterboncz/bloomfilter-bsd/blob/master/src/dtl/div.hpp,
      //  we might want to filter out certain generated dividers to gain extra speed
      // TODO: investigate branchless vs branchfull
      return {divisor};
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
   static constexpr forceinline T fastrange(const T& value, const T& n);

   /**
    * Reduces value to interval [0, 2^p]
    * @tparam T
    * @param value
    * @param p
    * @return
    */
   template<typename T>
   static constexpr forceinline T shift(const T& value, const unsigned char p = sizeof(T) * 8) {
      return value >> (sizeof(T) * 8 - p);
   }

   /**
    * Takes the lower 64 bits of a 128 bit hash value
    * @param value
    * @return
    */
   static constexpr forceinline HASH_64 lower_half(const HASH_128& value) {
      return value.lower;
   }

   /**
    * Takes the upper 64 bits of a 128 bit hash value
    * @param value
    * @return
    */
   static constexpr forceinline HASH_64 upper_half(const HASH_128& value) {
      return value.higher;
   }

   /**
    * xors the upper and lower 64-bits of a 128 bit value to obtain a final 64 bit hash
    * @param value
    * @return
    */
   static constexpr forceinline HASH_64 xor_both(const HASH_128& value) {
      return value.higher ^ value.lower;
   }

   /**
    * Hash 128 input bits down to 64 bits of output, intended to be a
    * reasonably good hash function.
    *
    * This code was taken from CityHash:
    * Copyright (c) 2011 Google, Inc.
    *
    * Permission is hereby granted, free of charge, to any person obtaining a copy
    * of this software and associated documentation files (the "Software"), to deal
    * in the Software without restriction, including without limitation the rights
    * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    * copies of the Software, and to permit persons to whom the Software is
    * furnished to do so, subject to the following conditions:
    *
    * The above copyright notice and this permission notice shall be included in
    * all copies or substantial portions of the Software.
    *
    * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    * THE SOFTWARE.
    */
   static constexpr forceinline HASH_64 hash_128_to_64(const HASH_128& x) {
      // Murmur-inspired hashing.
      const HASH_64 kMul = 0x9ddfea08eb382d69ULL;
      HASH_64 a = (x.lower ^ x.higher) * kMul;
      a ^= (a >> 47);
      HASH_64 b = (x.higher ^ a) * kMul;
      b ^= (b >> 47);
      b *= kMul;
      return b;
   }
};

template<>
constexpr forceinline HASH_32 HashReduction::fastrange(const HASH_32& value, const HASH_32& n) {
   return ((uint64_t) value * (uint64_t) n) >> 32;
}

template<>
constexpr forceinline HASH_64 HashReduction::fastrange(const HASH_64& value, const HASH_64& n) {
   return ((__uint128_t) value * (__uint128_t) n) >> 64;
}
