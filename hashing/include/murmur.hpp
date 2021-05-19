#pragma once

#include <string>

#include <convenience.hpp>

/**
 * Implementations taken from Austin Appleby's original code:
 * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530),
 * or the submodule hash_functions/murmur/src/MurmurHash3.cpp
 *
 * Code had to be copied from .cpp file to facilitate inlining.
 * This is afaik okay since Austin Appleby, the author, waived
 * any copyright (see header comment in MurmurHash3.cpp)
 */
template<class T>
struct MurmurFinalizer {
   static std::string name() {
      return "murmur_finalizer" + std::to_string(sizeof(T) * 8);
   }

   constexpr forceinline T operator()(T key) const;
};

template<>
constexpr HASH_32 MurmurFinalizer<HASH_32>::operator()(HASH_32 key) const {
   key ^= key >> 16;
   key *= 0x85ebca6bLU;
   key ^= key >> 13;
   key *= 0xc2b2ae35LU;
   key ^= key >> 16;

   return key;
}

template<>
constexpr HASH_64 MurmurFinalizer<HASH_64>::operator()(HASH_64 key) const {
   key ^= key >> 33;
   key *= 0xff51afd7ed558ccdLLU;
   key ^= key >> 33;
   key *= 0xc4ceb9fe1a85ec53LLU;
   key ^= key >> 33;

   return key;
}

/**
 * Murmur3 32-bit, adjusted to fixed 32-bit input values (compiler would presumably perform the same optimizations.
 * However, in this explicit form it is clear what computation actually happens. This might be important for the
 * argument "VLDB paper misrepresents murmur quality, here's what murmur actually computes:").
 *
 * @tparam seed murmur seed, defaults to 0x238EF8E3 (random 32-bit prime)
 * @return
 */
template<const HASH_32 seed = 0x238EF8E3LU>
struct Murmur3Hash32 {
   static std::string name() {
      return "murmur3_32";
   }

   constexpr forceinline HASH_32 operator()(const HASH_32& key) const {
      const auto len = sizeof(HASH_32);

      /// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530)
      const auto rotl32 = [](uint32_t x, int8_t r) { return (x << r) | (x >> (32 - r)); };

      // nblocks = len / 4 = sizeof(value) / 4  = 4 / 4 = 1

      uint32_t h1 = seed;

      uint32_t c1 = 0xcc9e2d51;
      uint32_t c2 = 0x1b873593;

      //----------
      // body

      // getblock(blocks, -1) = getblock((void*)(&value) + 1 * 4, -1) = getblock(&value + 1, -1) = (&value + 1)[-1] = *(&value + 1 - 1) = value
      uint32_t k1 = key;

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
      // finalizer
      return finalizer(h1 ^ len);
   }

  private:
   MurmurFinalizer<HASH_32> finalizer;
};

/**
 * Murmur3 128-bit, adjusted to fixed 64-bit input values (compiler would presumably perform the same optimizations.
 * However, in this explicit form it is clear what computation actually happens. This might be important for the
 * argument "VLDB paper misrepresents murmur quality, here's what murmur actually computes:").
 *
 * @param value
 * @param seed murmur seed, defaults to 0xC7455FEC83DD661F (random 64-bit prime)
 * @return
 */
template<const HASH_64 seed = 0xC7455FEC83DD661FLLU>
struct Murmur3Hash128 {
   static std::string name() {
      return "murmur3_128";
   }

   constexpr forceinline HASH_128 operator()(const HASH_64& key) const {
      // nblocks = len / 16 = sizeof(value) / 16  = 8 / 16 = 0 (int division)

      /// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp (commit: 61a0530)
      const auto rotl64 = [](uint64_t x, int8_t r) { return (x << r) | (x >> (64 - r)); };

      uint64_t h1 = seed;
      uint64_t h2 = seed;

      uint64_t c1 = 0x87c37b91114253d5LLU;
      uint64_t c2 = 0x4cf5ad432745937fLLU;

      //----------
      // body

      // since nblocks = 0, loop will never execute

      //----------
      // tail

      // nblocks = 0, data just points at value
      const auto* tail = (const uint8_t*) (&key);

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
      // finalizer

      h1 ^= sizeof(HASH_64);
      h2 ^= sizeof(HASH_64);

      h1 += h2;
      h2 += h1;

      h1 = finalizer(h1);
      h2 = finalizer(h2);

      h1 += h2;
      h2 += h1;

      return to_hash128(h1, h2);
   }

  private:
   MurmurFinalizer<HASH_64> finalizer;
};
