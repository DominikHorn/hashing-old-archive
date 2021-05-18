#pragma once

#include <random>

#include <convenience.hpp>

/**
 * This is purely meant as a baseline for the amount of collisions to expect, i.e.,
 * RandomHash does not depend on the key and will instead generate a uniform random
 * number within (0, n);
 */
template<typename T = uint64_t>
struct RandomHash {
   RandomHash(const T& hashtable_size, const T& seed = 0xC7455FEC83DD661FLLU)
      : gen(seed), dist(0, hashtable_size - 1){};

   static std::string name() {
      return "random_hash";
   }

   forceinline T operator()(const T& key) {
      UNUSED(key);
      return dist(gen);
   }

  private:
   std::default_random_engine gen;
   std::uniform_int_distribution<uint64_t> dist;
};
