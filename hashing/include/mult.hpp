#pragma once

#include <cassert>
#include <string>

#include <convenience.hpp>

/**
 * Multiplicative hashing, i.e., (x * constant % 2^w) >> (w - p)
 */
template<class T, const T constant, const char* base_name>
struct MultiplicationHash {
   static std::string name() {
      return base_name + std::to_string(sizeof(T) * 8);
   }

   /**
    * hash = (key * constant % 2^w) >> (w - p), where w = sizeof(T) * 8
    * i.e., take the highest p bits from the w-bit multiplication
    * key * constant (ignore overflow)
    *
    * @param key the key to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= sizeof(T)*8
    */
   constexpr forceinline T operator()(const T& key, const uint8_t p = sizeof(T) * 8) const {
      constexpr auto t = sizeof(T) * 8;
      assert(p >= 0 && p <= t);
      return (key * constant) >> (t - p);
   }
};

const char MULT_PRIME_32[] = "mult_prime32";
const char MULT_FIBONNACI_32[] = "mult_fibonnaci32";
const char MULT_FIBONNACI_PRIME_32[] = "mult_fibonnaci_prime32";
const char MULT_PRIME_64[] = "mult_prime64";
const char MULT_FIBONNACI_64[] = "mult_fibonnaci64";
const char MULT_FIBONNACI_PRIME_64[] = "mult_fibonnaci_prime64";

// TODO: see if there are "better" prime constants (if that is even possible). Especially investigate
// TODO: (?) test performance for random, (likely) non prime constant as suggested in http://www.vldb.org/pvldb/vol9/p96-richter.pdf

/// Multiplicative 32-bit hashing with prime constants
using PrimeMultiplicationHash32 = MultiplicationHash<HASH_32, 0x238EF8E3LU, MULT_PRIME_32>;
/// Multiplicative 64-bit hashing with prime constants
using PrimeMultiplicationHash64 = MultiplicationHash<HASH_64, 0xC7455FEC83DD661FLLU, MULT_PRIME_64>;

/// Multiplicative 32-bit hashing with constants derived from the golden ratio
using FibonacciHash32 = MultiplicationHash<HASH_32, 0x9E3779B9LU, MULT_FIBONNACI_32>;
/// Multiplicative 64-bit hashing with constants derived from the golden ratio
using FibonacciHash64 = MultiplicationHash<HASH_64, 0x9E3779B97F4A7C15LLU, MULT_FIBONNACI_64>;

/// Multiplicative 32-bit hashing with prime constants derived from the golden ratio
using FibonnaciPrimeHash32 = MultiplicationHash<HASH_32, 0x9e3779b1LU, MULT_FIBONNACI_PRIME_32>;
/// Multiplicative 64-bit hashing with prime constants derived from the golden ratio
using FibonnaciPrimeHash64 = MultiplicationHash<HASH_64, 0x9E3779B97F4A7C55LLU, MULT_FIBONNACI_PRIME_64>;
