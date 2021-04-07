#pragma once

/**
 * ----------------------------
 *      General & Typedefs
 * ----------------------------
 */
#define HASH_32 std::uint32_t
#define HASH_64 std::uint64_t
struct HASH_128 {
   HASH_64 lower;
   HASH_64 higher;
};
