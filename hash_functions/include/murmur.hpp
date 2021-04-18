#pragma once

#include <convenience.hpp>

/**
 * ----------------------------
 *      Murmur Hashing
 * ----------------------------
 *
 * Implementations taken from Austin Appleby's original code:
 * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530),
 * or the submodule hash_functions/murmur/src/MurmurHash3.cpp
 *
 * Code had to be copied from .cpp file to facilitate inlining.
 * This is afaik okay since Austin Appleby, the author, disclaimed
 * any copyright (see header comment in MurmurHash3.cpp)
 */
struct MurmurHash3 {
   /**
    * Murmur 32-bit finalizer
    *
    * Richter, Stefan, Victor Alvarez, and Jens Dittrich "A seven-dimensional
    * analysis of hashing methods and its implications on query processing."
    * (PVLDB 9.3 (2015): 96-107.) only use the finalizer, meaning they skip
    * some computations, which might be important for hash quality. Therefore:
    *
    * @param value the value to finalize
    * @return 32 bit hash
    */
   static constexpr forceinline HASH_32 finalize_32(HASH_32& value) {
      value ^= value >> 16;
      value *= 0x85ebca6blu;
      value ^= value >> 13;
      value *= 0xc2b2ae35lu;
      value ^= value >> 16;

      return value;
   }

   /**
    * Murmur 64-bit finalizer
    *
    * Richter, Stefan, Victor Alvarez, and Jens Dittrich "A seven-dimensional
    * analysis of hashing methods and its implications on query processing."
    * (PVLDB 9.3 (2015): 96-107.) only use the finalizer, meaning they skip
    * some computations, which might be important for hash quality. Therefore:
    *
    * @param value the value to finalize
    * @return 64 bit hash
    */
   static constexpr forceinline HASH_64 finalize_64(HASH_64 value) {
      value ^= value >> 33;
      value *= 0xff51afd7ed558ccdLLU;
      value ^= value >> 33;
      value *= 0xc4ceb9fe1a85ec53LLU;
      value ^= value >> 33;

      return value;
   }

   /**
    * Murmur3 32-bit, adjusted to fixed 32-bit input values (compiler would presumably perform the same optimizations.
    * However, in this explicit form it is clear what computation actually happens. This might be important for the
    * argument "VLDB paper misrepresents murmur quality, here's what murmur actually computes:").
    *
    * @param value
    * @param seed murmur seed, defaults to 0x238EF8E3 (random 32-bit prime)
    * @return
    */
   static constexpr forceinline HASH_32 murmur3_32(const HASH_32& value, const HASH_32& seed = 0x238EF8E3lu) {
      const auto len = sizeof(HASH_32);
      // nblocks = len / 4 = sizeof(value) / 4  = 4 / 4 = 1

      uint32_t h1 = seed;

      uint32_t c1 = 0xcc9e2d51;
      uint32_t c2 = 0x1b873593;

      //----------
      // body

      // getblock(blocks, -1) = getblock((void*)(&value) + 1 * 4, -1) = getblock(&value + 1, -1) = (&value + 1)[-1] = *(&value + 1 - 1) = value
      uint32_t k1 = value;

      k1 *= c1;
      k1 = rotl32(k1, 15);
      k1 *= c2;

      h1 ^= k1;
      h1 = rotl32(h1, 13);
      h1 = h1 * 5 * 0xe6546b64;

      //----------
      // tail

      // len & 3 = sizeof(value) & 3 = 4 & 3 = 0, therefore nothing happens for tail computation

      //----------
      // finalization

      h1 ^= len;
      h1 = finalize_32(h1);
      return h1;
   }

   /**
    * Murmur3 128-bit, adjusted to fixed 64-bit input values (compiler would presumably perform the same optimizations.
    * However, in this explicit form it is clear what computation actually happens. This might be important for the
    * argument "VLDB paper misrepresents murmur quality, here's what murmur actually computes:").
    *
    * @param value
    * @param seed murmur seed, defaults to 0xC7455FEC83DD661F (random 64-bit prime)
    * @return
    */
   static forceinline HASH_128 murmur3_128(const HASH_64& value,
                                           const HASH_64& seed = (HASH_64) 0xC7455FEC83DD661Fllu) {
      // nblocks = len / 16 = sizeof(value) / 16  = 8 / 16 = 0 (int division)

      uint64_t h1 = seed;
      uint64_t h2 = seed;

      uint64_t c1 = 0x87c37b91114253d5llu;
      uint64_t c2 = 0x4cf5ad432745937fllu;

      //----------
      // body

      // since nblocks = 0, loop will never execute

      //----------
      // tail

      // nblocks = 0, data just points at value
      const uint8_t* tail = (const uint8_t*) (&value);

      uint64_t k1 = 0;
      //      uint64_t k2 = 0;

      // len = sizeof(value) = 8
      k1 ^= uint64_t(tail[7]) << 56;
      k1 ^= uint64_t(tail[6]) << 48;
      k1 ^= uint64_t(tail[5]) << 40;
      k1 ^= uint64_t(tail[4]) << 32;
      k1 ^= uint64_t(tail[3]) << 24;
      k1 ^= uint64_t(tail[2]) << 16;
      k1 ^= uint64_t(tail[1]) << 8;
      k1 ^= uint64_t(tail[0]) << 0;
      k1 *= c1;
      k1 = rotl64(k1, 31);
      k1 *= c2;
      h1 ^= k1;

      //----------
      // finalization

      h1 ^= sizeof(HASH_64);
      h2 ^= sizeof(HASH_64);

      h1 += h2;
      h2 += h1;

      h1 = finalize_64(h1);
      h2 = finalize_64(h2);

      h1 += h2;
      h2 += h1;

      return {h2, h1};
   }

  private:
   /// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530)
   static constexpr forceinline uint32_t rotl32(uint32_t x, int8_t r) {
      return (x << r) | (x >> (32 - r));
   }

   /// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530)
   static constexpr forceinline uint64_t rotl64(uint64_t x, int8_t r) {
      return (x << r) | (x >> (64 - r));
   }
};
