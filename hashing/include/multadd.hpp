#pragma once

#include <cassert>
#include <convenience.hpp>

template<class T, class R, const R constant1, const R constant2, const char* base_name>
struct MultiplicationAddHash {
   static std::string name() {
      return base_name + std::to_string(sizeof(T) * 8);
   }

   /**
    * hash = ((key * constant1 + constant2) % 2^w) >> (w - p), where w = sizeof(T) * 8
    * i.e., take the highest p bits from the w-bit multiplication & addition
    * key * constant1 + constant2 (ignore overflow)
    *
    * @param key the key to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= sizeof(T)*8
    */
   constexpr forceinline T operator()(const T& key, const uint8_t p = sizeof(T) * 8) const {
      constexpr auto t = sizeof(T) * 8;
      assert(p >= 0 && p <= t);
      return (key * constant1 + constant2) >> (t - p);
   }
};

const char MA32[] = "mult_add32";
const char MA64[] = "mult_add64";

/// 32-bit MultAdd hashing
using MultAddHash32 = MultiplicationAddHash<HASH_32, HASH_64, 0xC16FD7EEC0213493LLU, 0xA501000042C9A6E3LLU, MA32>;

/**
 * 64-bit MultAdd hashing
 *
 * NOTE: this hashing method requires 128 bit arithmetic. If the target CPU does not
 * natively support this, gcc/clang will generate assembly that emulates true
 * 128-bit arithmetic using 64-bit instructions. This incurs an overhead that
 * is likely really detremental to the algorithms performance.
 */
using MultAddHash64 =
   MultiplicationAddHash<HASH_64, unsigned __int128,
                         (static_cast<unsigned __int128>(0x9B6E895FDDB88E31) << 64) | 0x09E77036171A861D,
                         (static_cast<unsigned __int128>(0xF89E2E1E25B51473) << 64) | 0x2113E4015584C8AF, MA64>;