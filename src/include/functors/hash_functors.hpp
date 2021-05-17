#pragma once

#include <hashing.hpp>

struct Murmur3FinalizerFunc {
   static std::string name() {
      return "murmur3_fin64";
   }

   forceinline HASH_64 operator()(const HASH_64& key) const {
      return MurmurHash3::finalize_64(key);
   }
};

struct Murmur3FinalizerCuckoo2Func {
   static std::string name() {
      return "murmur3_fin64_2";
   }

   forceinline HASH_64 operator()(const HASH_64& key, const HASH_64& h1) const {
      return MurmurHash3::finalize_64(key ^ h1);
   }
};

struct AquaLowFunc {
   static std::string name() {
      return "aqua_low";
   }

   forceinline HASH_64 operator()(const HASH_64& key) const {
      return AquaHash::hash64(key);
   }
};
