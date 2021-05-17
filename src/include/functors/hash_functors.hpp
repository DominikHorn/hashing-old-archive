#pragma once

#include <hashing.hpp>

struct Murmur3FinalizerCuckoo2Func {
   static std::string name() {
      return "murmur3_fin64_2";
   }

   forceinline HASH_64 operator()(const HASH_64& key, const HASH_64& h1) const {
      return fin(key ^ h1);
   }

  private:
   MurmurFinalizer<HASH_64> fin;
};

struct AquaLowFunc {
   static std::string name() {
      return "aqua_low";
   }

   forceinline HASH_64 operator()(const HASH_64& key) const {
      return AquaHash::hash64(key);
   }
};
