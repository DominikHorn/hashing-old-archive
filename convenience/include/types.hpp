#pragma once

/**
 * ----------------------------
 *      General & Typedefs
 * ----------------------------
 */
#define HASH_32 std::uint32_t
#define HASH_64 std::uint64_t
#define HASH_128 unsigned __int128

constexpr forceinline HASH_128 to_hash128(HASH_64 higher, HASH_64 lower) {
   return (static_cast<HASH_128>(higher) << 64) | lower;
}