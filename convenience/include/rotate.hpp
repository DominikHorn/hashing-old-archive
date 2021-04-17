#pragma once

#include <assert.h>

/**
 * Rotate left instruction. Smart compilers will emit native rotate
 * instructions where available
 *
 * @tparam T
 * @param n number to rotate
 * @param c amount of bits to rotate by
 * @return
 */
template<typename T>
static constexpr forceinline T rotl(const T& n, unsigned int c) {
   const unsigned int mask = (8 * sizeof(T) - 1); // assumes width is a power of 2.
   assert((c <= mask) && "rotate by type width or more");
   c &= mask;
   return (n << c) | (n >> ((-c) & mask));
}

/**
 * Rotate right instruction. Smart compilers will emit native rotate
 * instructions where available
 *
 * @tparam T
 * @param n number to rotate
 * @param c amount of bits to rotate by
 * @return
 */
template<typename T>
static constexpr forceinline T rotr(const T& n, unsigned int c) {
   const unsigned int mask = (8 * sizeof(T) - 1); // assumes width is a power of 2.
   assert((c <= mask) && "rotate by type width or more");
   c &= mask;
   return (n >> c) | (n << ((-c) & mask));
}
