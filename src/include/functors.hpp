#pragma once

#include <hashing.hpp>

template<typename T>
struct FastrangeFunc {
   static std::string name() {
      return "fastrange" + std::to_string(sizeof(T) * 8);
   }
   forceinline HASH_64 operator()(const HASH_64& hash, const size_t& N) const {
      return Reduction::fastrange(hash, N);
   }
};

template<typename T>
struct FastModuloFunc {
  private:
   const libdivide::divider<T> magic_div;
   const size_t n;

  public:
   FastModuloFunc(const size_t& hashtable_size)
      : magic_div(Reduction::make_magic_divider(static_cast<HASH_64>(hashtable_size))), n(hashtable_size) {}

   static std::string name() {
      return "fast_modulo";
   }

   forceinline T operator()(const T& hash, const size_t& N) const {
      return Reduction::magic_modulo(hash, N, magic_div);
   }
};

struct Mult64Func {
   static std::string name() {
      return "mult64";
   }

   forceinline HASH_64 operator()(const HASH_64& key) const {
      return MultHash::mult64_hash(key);
   }
};

struct MultAdd64Func {
   static std::string name() {
      return "multadd64";
   }

   forceinline HASH_64 operator()(const HASH_64& key) const {
      return MultAddHash::multadd64_hash(key);
   }
};

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
      return "murmur3_fin64";
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
