#pragma once

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
      #define likely(expr)
      #define unlikely(expr)
   #endif

   #define neverinline __attribute__((noinline))
   #define packed __attribute__((packed))
   #define prefetch(address, mode, locality) __builtin_prefetch(address, mode, locality)
#else
   #error GNUC is the only supported compiler atm
#endif

struct Prefetcher {
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
   template<Mode mode = WRITE, Locality locality = NONE, size_t cache_line_size = 64>
   static forceinline unroll_loops void prefetch_block(const void* address, const size_t size) {
      for (auto ptr = (uint8_t*) address; ptr < (uint8_t*) address + size; ptr += cache_line_size) {
         prefetch(ptr, mode, locality);
      }
   }
};