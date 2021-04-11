#pragma once

#include <cassert>

#include "../convenience/convenience.hpp"
#include "types.hpp"

/**
 * ----------------------------
 *      MultAdd Hashing
 * ----------------------------
 */
struct MultAddHash {
   /**
    * hash = ((value * a + b) % 2^64) >> (64 - p)
    *
    * @param value the value to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= 32
    * @param a magic hash constant. You should choose a \in [0,2^64] and a is prime. Defaults to 0xC16FD7EEC0213493
    * @param b magic hash constant. You should choose b \in [0,2^64] and b is prime. Defaults to 0xA501000042C9A6E3
    */
   static constexpr forceinline HASH_32 multadd32_hash(const HASH_32& value,
                                                       const unsigned char p = 32,
                                                       const HASH_64& a = (HASH_64) 0xC16FD7EEC0213493llu,
                                                       const HASH_64& b = (HASH_64) 0xA501000042C9A6E3llu) {
      assert(p >= 0 && p <= 32);
      return ((uint64_t) value * a + b) >> (64 - p);
   }

   /**
    * hash = ((value * a + b) % 2^128) >> (128 - p)
    *
    * NOTE: this hashing method uses 128 bit arithmetic. If the target CPU does not
    * natively support this, gcc/clang will generate assembly that emulates true
    * 128-bit arithmetic using 64-bit instructions. This incurs an overhead that
    * is likely really detremental to the algorithms performance.
    *
    * @param value the value to hash
    * @param p how many bits the result should have, i.e., result value \in [0, 2^p].
    *   NOTE: 0 <= p <= 64
    * @param a magic hash constant. You should choose a \in [0,2^128] and a is prime. Defaults to 0x9B6E895FDDB88E3109E77036171A861D
    * @param b magic hash constant. You should choose b \in [0,2^128] and b is prime. Defaults to 0xF89E2E1E25B514732113E4015584C8AF
    */
   static constexpr forceinline HASH_64 multadd64_hash(const HASH_64& value,
                                                       const unsigned char p = 64,
                                                       const HASH_64& a_low = (HASH_64) 0x09E77036171A861Dllu,
                                                       const HASH_64& a_high = (HASH_64) 0x9B6E895FDDB88E31llu,
                                                       const HASH_64& b_low = (HASH_64) 0x2113E4015584C8AFllu,
                                                       const HASH_64& b_high = (HASH_64) 0xF89E2E1E25B51473llu) {
      assert(p >= 0 && p <= 64);
      const __uint128_t a = (__uint128_t) a_low + ((__uint128_t) a_high << 64);
      const __uint128_t b = (__uint128_t) b_low + ((__uint128_t) b_high << 64);
      return ((__uint128_t) value * a + b) >> (128 - p);
   }
};
