#pragma once

#include <cstddef>
#include <cstdint>
#include <x86intrin.h>

#include "builtins.hpp"

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
