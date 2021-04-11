#pragma once

// enable full inlining for xxhash code
#define XXH_INLINE_ALL
#include "xxHash/xxhash.h"

#include "../convenience/convenience.hpp"
#include "types.hpp"

/**
 *
 * ----------------------------
 *           xxHash
 * ----------------------------
 */

// TODO: benchmark with clang & gcc builds as clang supposedly improves xxHash performance
struct XXHash {
   template<typename T>
   static constexpr forceinline HASH_32 XXH32_hash(const T& value) {
      return XXH32(&value, sizeof(value), 0);
   }

   template<typename T>
   static constexpr forceinline HASH_32 XXH32_hash_withSeed(const T& value, HASH_32& seed) {
      return XXH32(&value, sizeof(value), seed);
   }

   template<typename T>
   static constexpr forceinline HASH_64 XXH64_hash(const T& value) {
      return XXH64(&value, sizeof(value), 0);
   }

   template<typename T>
   static constexpr forceinline HASH_64 XXH64_hash_withSeed(const T& value, HASH_64& seed) {
      return XXH64(&value, sizeof(value), seed);
   }

   template<typename T>
   static constexpr forceinline HASH_64 XXH3_hash(const T& value) {
      return XXH3_64bits(&value, sizeof(T));
   }

   template<typename T>
   static constexpr forceinline HASH_64 XXH3_hash_withSeed(const T& value, HASH_64& seed) {
      return XXH3_64bits_withSeed(&value, sizeof(T), seed);
   }

   template<typename T>
   static constexpr forceinline HASH_128 XXH3_128_hash(const T& value) {
      // TODO: is this the proper 128 bit XXH variant?
      const auto val = XXH3_128bits(&value, sizeof(T));
      return {.lower = val.low64, .higher = val.high64};
   }

   template<typename T>
   static constexpr forceinline HASH_128 XXH3_128_hash_withSeed(const T& value, HASH_64& seed) {
      // TODO: is this the proper 128 bit XXH variant?
      const auto val = XXH3_128bits_withSeed(&value, sizeof(T), seed);
      return {.lower = val.low64, .higher = val.high64};
   }
};
