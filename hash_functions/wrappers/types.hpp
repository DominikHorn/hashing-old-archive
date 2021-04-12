#pragma once

#include "convenience.hpp"

/**
 * ----------------------------
 *      General & Typedefs
 * ----------------------------
 */
#define HASH_32 std::uint32_t
#define HASH_64 std::uint64_t

// TODO: investigate using simd 128 bit values (e.g., __m128i)
struct HASH_128 {
   HASH_64 lower;
   HASH_64 higher;

   HASH_128(const uint64_t lower = 0, const uint64_t higher = 0) : lower(lower), higher(higher){};
} __attribute__((aligned(8)));
