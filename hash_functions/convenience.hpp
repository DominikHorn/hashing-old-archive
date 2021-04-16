#pragma once

#include <cstddef>
#include <cstdint>
#include <assert.h>

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
   #include <x86intrin.h>
#endif

/// prevent unused warnings
#define UNUSED(x) (void) (x)

#ifdef __GNUC__
   /**
    * These optimizing compiler hints were taken from
    * https://github.com/toschmidt/cpp-utility/blob/master/src/cpp-utility/compiler/CompilerHints.hpp#L8
    * (commit: 46a5c6b)
    *
    * We only want to perform them in release builds
    */
   #ifdef NDEBUG
      #define forceinline inline __attribute__((always_inline))
      #define unroll_loops __attribute__((optimize("unroll-loops")))
      #define likely(expr) __builtin_expect((expr), 1)
      #define unlikely(expr) __builtin_expect((expr), 0)
   #else
      #define forceinline
      #define unroll_loops
      #define likely(expr) expr
      #define unlikely(expr) expr
   #endif

   #define neverinline __attribute__((noinline))
   #define packed __attribute__((packed))
   #define prefetch(address, mode, locality) __builtin_prefetch(address, mode, locality)
#else
// TODO: we will need clang support, therefore verify that the above primitives all work with clang
   #error GNUC is the only supported compiler atm
#endif

/**
 * Memory barrier code taken from google-benchmark
 * See google-benchmark.license.txt for the code's apache license
 * statement. License is not provided inline since it is rather long.
 *
 * NOTE: this code is slightly modified from its original source!
 */
namespace Barrier {
#if defined(__GNUC__) || defined(__clang__)
   template<class Tp>
   forceinline void DoNotOptimize(Tp const& value) {
      asm volatile("" : : "r,m"(value) : "memory");
   }

   template<class Tp>
   forceinline void DoNotOptimize(Tp& value) {
   #if defined(__clang__)
      asm volatile("" : "+r,m"(value) : : "memory");
   #else
      asm volatile("" : "+m,r"(value) : : "memory");
   #endif
   }
#endif
} // namespace Barrier

struct Cache {
   enum Locality
   {
      NONE = 0,
      LOW = 1,
      MEDIUM = 2,
      HIGH = 3
   };
   enum Mode
   {
      READ = 0,
      WRITE = 1
   };

   /**
    * Prefetches a cache line starting at address, if possible on CPU
    */
   template<Mode mode = WRITE, Locality locality = NONE>
   static forceinline void prefetch_address(const void* address) {
      prefetch(address, mode, locality);
   }

   /**
    * Prefetches a block of data starting at address with given size.
    * NOTE: does not guarantee that the entire block actually is in cache afterwards
    * (missing prefetch instruction, evictions we don't control etc)
    *
    * @tparam mode hint for the compiler how the prefetched memory will be used. Either read or write
    * @tparam locality hint for the compiler
    * @tparam cache_line_size cache line size in bytes. Defaults to 64
    */
   template<Mode mode = WRITE, Locality locality = NONE,
            size_t cache_line_size = 64> // TODO: extract these into defines!
   static forceinline void prefetch_block(const void* address, const size_t size) {
      for (auto* ptr = (std::uint8_t*) address; ptr < (std::uint8_t*) address + size; ptr += cache_line_size) {
         prefetch(ptr, mode, locality);
      }
   }

   /**
    * Clears the cache of all addresses contained in [start, start+size]
    * @param start
    * @param size
    */
   template<size_t cache_line_size = 64>
   static forceinline void clearcache(const void* start, const size_t size) {
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
      const auto* addr = reinterpret_cast<const std::uint8_t*>(start);
      for (const auto* ptr = addr; ptr < addr + size; ptr += cache_line_size) {
         _mm_clflush(start);
      }
#else
   #error "clearcache() not implemented on your system"
#endif
   }
};

/**
 * Rotate left instruction. Smart compilers will emit native rotate
 * instructions where available
 *
 * @tparam T
 * @param n number to rotate
 * @param c amount of bits to rotate by
 * @return
 */
template<typename T>
static constexpr forceinline T rotl(const T& n, unsigned int c) {
   const unsigned int mask = (8 * sizeof(T) - 1); // assumes width is a power of 2.
   assert((c <= mask) && "rotate by type width or more");
   c &= mask;
   return (n << c) | (n >> ((-c) & mask));
}

/**
 * Rotate right instruction. Smart compilers will emit native rotate
 * instructions where available
 *
 * @tparam T
 * @param n number to rotate
 * @param c amount of bits to rotate by
 * @return
 */
template<typename T>
static constexpr forceinline T rotr(const T& n, unsigned int c) {
   const unsigned int mask = (8 * sizeof(T) - 1); // assumes width is a power of 2.
   assert((c <= mask) && "rotate by type width or more");
   c &= mask;
   return (n >> c) | (n << ((-c) & mask));
}

/**
 * Computes num / reference with extra precision
 * @tparam T
 * @param num
 * @param reference
 * @return
 */
template<typename T, typename K>
static constexpr forceinline long double relative_to(const T& num, const K& reference) {
   return static_cast<long double>(num) / static_cast<long double>(reference);
}

/**
 * Converts nanoseconds to seconds
 * @tparam T
 * @param num
 * @return
 */
template<typename T>
static constexpr forceinline long double nanoseconds_to_seconds(const T& ns) {
   return relative_to(ns, 1000000000.0);
}