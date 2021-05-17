#pragma once

#include <string>

#include <learned_models.hpp>

template<typename T, size_t Epsilon, size_t EpsilonRecursive>
struct PGMHashFunc {
  private:
   const pgm::PGMHash<T, Epsilon, EpsilonRecursive> model;
   const size_t N;

  public:
   template<typename RandomIt>
   PGMHashFunc(const RandomIt& first, const RandomIt& last, const size_t full_size)
      : model(first, last), N(full_size) {}

   static std::string name() {
      return "pgm_hash_eps" + std::to_string(Epsilon) + "_epsrec" + std::to_string(EpsilonRecursive);
   }

   forceinline T operator()(const T& key) const {
      return model.hash(key, N);
   }
};