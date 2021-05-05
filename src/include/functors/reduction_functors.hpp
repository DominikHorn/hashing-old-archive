#pragma once

#include <convenience.hpp>
#include <reduction.hpp>

template<typename T>
struct FastrangeFunc {
  private:
   const size_t N;

  public:
   FastrangeFunc(const size_t& num_buckets) : N(num_buckets){};

   static std::string name() {
      return "fastrange" + std::to_string(sizeof(T) * 8);
   }

   forceinline HASH_64 operator()(const HASH_64& hash) const {
      return Reduction::fastrange(hash, N);
   }
};

template<typename T>
struct FastModuloFunc {
  private:
   const libdivide::divider<T> magic_div;
   const size_t N;

  public:
   FastModuloFunc(const size_t& num_buckets)
      : magic_div(Reduction::make_magic_divider(static_cast<HASH_64>(num_buckets))), N(num_buckets) {}

   static std::string name() {
      return "fast_modulo";
   }

   forceinline T operator()(const T& hash) const {
      return Reduction::magic_modulo(hash, N, magic_div);
   }
};
