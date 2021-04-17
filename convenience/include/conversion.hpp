#pragma once

#include "builtins.hpp"

/**
 * Computes num / reference with extra precision
 * @tparam T
 * @param num
 * @param reference
 * @return
 */
template<typename T, typename K>
static constexpr forceinline long double relative_to(const T& num, const K& reference) {
   return static_cast<long double>(num) / static_cast<long double>(reference);
}

/**
 * Converts nanoseconds to seconds
 * @tparam T
 * @param num
 * @return
 */
template<typename T>
static constexpr forceinline long double nanoseconds_to_seconds(const T& ns) {
   return relative_to(ns, 1000000000.0);
}
