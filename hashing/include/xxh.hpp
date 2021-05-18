#pragma once

#include <string>

#include <convenience.hpp>
#include <thirdparty/xxhash.h>

#ifndef __clang__
   #warning "xxHash is supposedly faster with clang"
#endif

template<class T, const HASH_32 seed = 0>
struct XXHash32 {
   static std::string name() {
      return "xxh32" + (seed != 0 ? "_seed_" + std::to_string(seed) : "");
   }

   forceinline HASH_32 operator()(const T& data) const {
      return _XXHash::XXH32(&data, sizeof(T), seed);
   }
};

template<class T, const HASH_32 seed = 0>
struct XXHash64 {
   static std::string name() {
      return "xxh64" + (seed != 0 ? "_seed_" + std::to_string(seed) : "");
   }

   forceinline HASH_64 operator()(const T& data) const {
      return _XXHash::XXH64(&data, sizeof(T), seed);
   }
};

template<class T>
struct XXHash3 {
   static std::string name() {
      return "xxh3";
   }

   forceinline HASH_64 operator()(const T& data) const {
      return _XXHash::XXH3_64bits(&data, sizeof(T));
   }
};

/**
 * XXH3 with seed, supposedly slower
 * @tparam T
 */
template<class T, const HASH_64 seed>
struct XXHash3Seeded {
   static std::string name() {
      return "xxh3_seed_" + std::to_string(seed);
   }

   forceinline HASH_64 operator()(const T& data) const {
      return _XXHash::XXH3_64bits_withSeed(&data, sizeof(T), seed);
   }
};

template<class T>
struct XXHash3_128 {
   static std::string name() {
      return "xxh3_128";
   }

   forceinline HASH_128 operator()(const T& data) const {
      const auto val = _XXHash::XXH3_128bits(&data, sizeof(T));
      return to_hash128(val.high64, val.low64);
   }
};

template<class T, const HASH_64 seed>
struct XXHash3_128Seeded {
   static std::string name() {
      return "xxh3_128_seed_" + std::to_string(seed);
   }

   forceinline HASH_128 operator()(const T& data) const {
      const auto val = _XXHash::XXH3_128bits_withSeed(&data, sizeof(T), seed);
      return to_hash128(val.high64, val.low64);
   }
};