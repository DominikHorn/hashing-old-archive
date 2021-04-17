#pragma once

#include <convenience.hpp>
#include <thirdparty/xxhash.h>

#include "types.hpp"

/**
 *
 * ----------------------------
 *           xxHash
 * ----------------------------
 */
// TODO: benchmark with clang & gcc builds as clang supposedly improves xxHash performance
struct XXHash {
    template<typename T>
    static constexpr forceinline HASH_32 XXH32_hash(const T& value) {
        return _XXHash::XXH32(&value, sizeof(value), 0);
    }

    template<typename T>
    static constexpr forceinline HASH_32 XXH32_hash_withSeed(const T& value, HASH_32& seed) {
        return _XXHash::XXH32(&value, sizeof(value), seed);
    }

    template<typename T>
    static constexpr forceinline HASH_64 XXH64_hash(const T& value) {
        return _XXHash::XXH64(&value, sizeof(value), 0);
    }

    template<typename T>
    static constexpr forceinline HASH_64 XXH64_hash_withSeed(const T& value, HASH_64& seed) {
        return _XXHash::XXH64(&value, sizeof(value), seed);
    }

    template<typename T>
    static constexpr forceinline HASH_64 XXH3_hash(const T& value) {
        return _XXHash::XXH3_64bits(&value, sizeof(T));
    }

    template<typename T>
    static constexpr forceinline HASH_64 XXH3_hash_withSeed(const T& value, HASH_64& seed) {
        return _XXHash::XXH3_64bits_withSeed(&value, sizeof(T), seed);
    }

    template<typename T>
    static forceinline HASH_128 XXH3_128_hash(const T& value) {
        // TODO: is this the proper 128 bit XXH variant?
        const auto val = _XXHash::XXH3_128bits(&value, sizeof(T));
        return {val.low64, val.high64};
    }

    template<typename T>
    static forceinline HASH_128 XXH3_128_hash_withSeed(const T& value, HASH_64& seed) {
        // TODO: is this the proper 128 bit XXH variant?
        const auto val = _XXHash::XXH3_128bits_withSeed(&value, sizeof(T), seed);
        return {val.low64, val.high64};
    }
};