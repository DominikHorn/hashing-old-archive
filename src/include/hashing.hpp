#ifndef HASHING_H
#define HASHING_H

typedef unsigned long HASH_32;
typedef unsigned long long HASH_64;
typedef struct {
    HASH_64 low64;
    HASH_64 high64;
} HASH_128;

#endif