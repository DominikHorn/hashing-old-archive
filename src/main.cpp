#include <iostream>
#include <vector>

#include "hashing.hpp"
#include "xxHash.hpp"

int main() {
    // TODO: temporary test
    for (unsigned long long i = 0; i < 32; i++) {
        std::cout << i << " |--xxHash--> " << XXH64_hash(i) << std::endl;
    }

    return 0;
}
