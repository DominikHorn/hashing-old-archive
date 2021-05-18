#pragma once

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
   #include <x86intrin.h>
#else
   #error "Your compiler is not supported"
#endif

// TODO(dominik): utilize this for batching!
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
namespace Reduction {
   /**
    * NOOP reduction, i.e., doesn't do anything
    */
   template<class T>
   struct DoNothing {
      explicit DoNothing(const size_t& num_buckets) {
         UNUSED(num_buckets);
      }

      static std::string name() {
         return "do_nothing";
      }

      constexpr forceinline T operator()(const T& hash) const {
         return hash;
      }
   };

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
   template<class T>
   struct Modulo {
      explicit Modulo(const size_t& num_buckets) : N(num_buckets) {}

      static std::string name() {
         return "modulo";
      }

      constexpr forceinline T operator()(const T& hash) const {
         return hash % N;
      }

     private:
      const size_t N;
   };

   template<typename T>
   struct FastModulo {
     private:
      const libdivide::divider<T> magic_div;
      const size_t N;

     public:
      // TODO(dominik): similar to https://github.com/peterboncz/bloomfilter-bsd/blob/master/src/dtl/div.hpp,
      //  we might want to filter out certain generated dividers to gain extra speed
      explicit FastModulo(const size_t& num_buckets) : magic_div({static_cast<T>(num_buckets)}), N(num_buckets) {}

      static std::string name() {
         return "fast_modulo";
      }

      forceinline T operator()(const T& hash) const {
         // TODO(dominik): investigate SIMD batching opportunities (see libdivide include for different options)

         const auto div = hash / magic_div; // Operator overloading ensures this is not an actual division
         const auto remainder = hash - div * N;
         assert(remainder < N);
         return remainder;
      }
   };

   template<typename T>
   struct BranchlessFastModulo {
     private:
      const libdivide::branchfree_divider<T> magic_div;
      const size_t N;

     public:
      // TODO(dominik): similar to https://github.com/peterboncz/bloomfilter-bsd/blob/master/src/dtl/div.hpp,
      //  we might want to filter out certain generated dividers to gain extra speed
      explicit BranchlessFastModulo(const size_t& num_buckets)
         : magic_div({static_cast<T>(num_buckets)}), N(num_buckets) {}

      static std::string name() {
         return "branchless_fast_modulo";
      }

      forceinline T operator()(const T& hash) const {
         // TODO(dominik): investigate SIMD batching opportunities (see libdivide include for different options)

         const auto div = hash / magic_div; // Operator overloading ensures this is not an actual division
         const auto remainder = hash - div * N;
         assert(remainder < N);
         return remainder;
      }
   };

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
   template<class T>
   struct Fastrange {
      explicit Fastrange(const size_t& num_buckets) : N(num_buckets){};

      static std::string name() {
         return "fastrange" + std::to_string(sizeof(T) * 8);
      }

      constexpr forceinline T operator()(const T& hash) const;

     private:
      const size_t N;
   };

   template<typename T>
   struct MinMaxCutoff {
      explicit MinMaxCutoff(const size_t& num_buckets) : N(num_buckets) {}

      static std::string name() {
         return "min_max_cutoff";
      }

      /**
       * min/max clamps the value. Optimized for values that are very likely inside of bounds
       * @tparam T
       * @param value
       * @param N
       * @return
       */
      forceinline T operator()(const T& hash) const {
         if (unlikely(hash < 0))
            return 0;
         if (unlikely(hash >= N))
            return N - 1;

         return hash;
      }

     private:
      const size_t N;
   };

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
   static constexpr forceinline HASH_64 lower(const HASH_128& value) {
      return value & 0xFFFFFFFFFFFFFFFFLLU;
   }

   template<class Hashfn>
   struct Lower {
      static std::string name() {
         return Hashfn::name() + "_low";
      }

      template<class Data>
      forceinline HASH_64 operator()(const Data& key) const {
         const HASH_128 value = hashfn(key);
         return lower(value);
      }

     private:
      Hashfn hashfn;
   };

   /**
    * Takes the upper 64 bits of a 128 bit hash value
    * @param value
    * @return
    */
   static constexpr forceinline HASH_64 higher(const HASH_128& value) {
      return value >> 64;
   }

   template<class Hashfn>
   struct Higher {
      static std::string name() {
         return Hashfn::name() + "_upp";
      }

      template<class Data>
      forceinline HASH_64 operator()(const Data& key) const {
         const HASH_128 value = hashfn(key);
         return higher(value);
      }

     private:
      Hashfn hashfn;
   };

   /**
    * xors the upper and lower 64-bits of a 128 bit value to obtain a final 64 bit hash
    * @param value
    * @return
    */
   static constexpr forceinline HASH_64 xor_both(const HASH_128& value) {
      return lower(value) ^ higher(value);
   }

   template<class Hashfn>
   struct Xor {
      static std::string name() {
         return Hashfn::name() + "_xor";
      }

      template<class Data>
      forceinline HASH_64 operator()(const Data& key) const {
         const HASH_128 value = hashfn(key);
         return xor_both(value);
      }

     private:
      Hashfn hashfn;
   };

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
      HASH_64 a = (lower(x) ^ higher(x)) * kMul;
      a ^= (a >> 47);
      HASH_64 b = (higher(x) ^ a) * kMul;
      b ^= (b >> 47);
      b *= kMul;
      return b;
   }

   template<class Hashfn>
   struct City {
      static std::string name() {
         return Hashfn::name() + "_city";
      }

      template<class Data>
      forceinline HASH_64 operator()(const Data& key) const {
         const HASH_128 value = hashfn(key);
         return hash_128_to_64(value);
      }

     private:
      Hashfn hashfn;
   };

   /**
    * Extract 32 bits using __mm_extract_epi32
    *
    * @tparam A
    * @tparam select bit offset according to formula select * 32. Defaults to 0
    * @param a
    * @return
    */
   template<uint8_t select = 0, typename A>
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
   template<uint8_t select = 0, typename A>
   static constexpr forceinline HASH_64 extract_64(const A& a) {
      return _mm_extract_epi64(a, select);
   }
}; // namespace Reduction

template<>
constexpr forceinline HASH_32 Reduction::Fastrange<HASH_32>::operator()(const HASH_32& value) const {
   return static_cast<HASH_32>((static_cast<uint64_t>(value) * static_cast<uint64_t>(N)) >> 32);
}

template<>
constexpr forceinline HASH_64 Reduction::Fastrange<HASH_64>::operator()(const HASH_64& value) const {
#ifdef __SIZEOF_INT128__
   return static_cast<HASH_64>((static_cast<__uint128_t>(value) * static_cast<__uint128_t>(N)) >> 64);
#else
   #warning \
      "Fastrange fallback (actual modulo) active, since 128bit integer multiplication seems to be unsupported on this system/compiler"
   return value % n; // Fallback
#endif
}