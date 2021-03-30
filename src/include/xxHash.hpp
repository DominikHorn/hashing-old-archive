#ifndef XXHASH_H
#define XXHASH_H

#include "../../opensource_references/xxHash/xxhash.h"
#include "hashing.hpp"

template <typename T>
HashValue xxHash(T value, XXH64_hash_t seed = 1234) {
    return XXH64(&value, sizeof(T), seed);
}

#endif