#pragma once

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
   #include <x86intrin.h>
#else
   #error "Your compiler is not supported"
#endif

// TODO: utilize this for batching!
//#define LIBDIVIDE_SSE2
//#define LIBDIVIDE_AVX2
//#define LIBDIVIDE_AVX512
//#define LIBDIVIDE_NEON
#include <thirdparty/libdivide.h>

#include <convenience.hpp>

/**
 * Implements different reducers to map values from
 * [0,2^d] into [0, N]
 *
 */
struct Reduction {
   /**
    * NOOP reduction, i.e., doesn't do anything
    */
   template<typename T>
   static constexpr forceinline T do_nothing(const T& value, const T& n) {
      UNUSED(n);
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
   template<typename T, typename R>
   static constexpr forceinline T modulo(const T& value, const R& n) {
      return value % n;
   }

   template<typename T, typename R, typename Divider = libdivide::divider<T>>
   static constexpr forceinline T magic_modulo(const T& value, const R& n, const Divider& fast_d) {
      // TODO: investigate SIMD batching opportunities (see libdivide include for different options)

      const auto div = value / fast_d; // Operator overloading ensures this is not an actual division
      const auto remainder = value - div * n;
      assert(remainder < n);
      return remainder;
   }

   template<typename T>
   static forceinline libdivide::divider<T> make_magic_divider(const T& divisor) {
      // TODO: similar to https://github.com/peterboncz/bloomfilter-bsd/blob/master/src/dtl/div.hpp,
      //  we might want to filter out certain generated dividers to gain extra speed
      return {divisor};
   }

   template<typename T>
   static forceinline libdivide::branchfree_divider<T> make_branchfree_magic_divider(const T& divisor) {
      // TODO: similar to https://github.com/peterboncz/bloomfilter-bsd/blob/master/src/dtl/div.hpp,
      //  we might want to filter out certain generated dividers to gain extra speed
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
   static constexpr forceinline T fastrange(const T& value, const size_t& n);

   /**
    * min/max cuts the value
    * @tparam T
    * @param value
    * @param N
    * @return
    */
   template<typename T>
   static constexpr forceinline T min_max_cutoff(const T& value, const size_t& N) {
      return std::max(static_cast<T>(0), std::min(value, static_cast<T>(N - 1)));
   }

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

   /**
    * Extract 32 bits using __mm_extract_epi32
    *
    * @tparam A
    * @tparam select bit offset according to formula select * 32. Defaults to 0
    * @param a
    * @return
    */
   template<unsigned short select = 0, typename A>
   static constexpr forceinline HASH_32 extract_32(const A& a) {
      return _mm_extract_epi32(a, select);
   }

   /**
    * Extract 64 bits using __mm_extract_epi64
    *
    * @tparam A
    * @tparam select either 0 or 1 (low/high selection). Defaults to 0
    * @param a
    * @return
    */
   template<unsigned short select = 0, typename A>
   static constexpr forceinline HASH_64 extract_64(const A& a) {
      return _mm_extract_epi64(a, select);
   }
};

template<>
constexpr forceinline HASH_32 Reduction::fastrange(const HASH_32& value, const size_t& n) {
   return static_cast<HASH_32>((static_cast<uint64_t>(value) * static_cast<uint64_t>(n)) >> 32);
}

template<>
constexpr forceinline HASH_64 Reduction::fastrange(const HASH_64& value, const size_t& n) {
#ifdef __SIZEOF_INT128__
   return static_cast<HASH_64>((static_cast<__uint128_t>(value) * static_cast<__uint128_t>(n)) >> 64);
#else
   #warning \
      "Fastrange fallback (actual modulo) active, since 128bit integer multiplication seems to be unsupported on this system/compiler"
   return value % n; // Fallback
#endif
}
