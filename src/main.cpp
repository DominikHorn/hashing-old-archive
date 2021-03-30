#include <iostream>
#include <vector>

#include "hashing.hpp"
#include "xxHash.hpp"

int main() {
    // TODO: temporary test
    for (HashValue i = 0; i < 32; i++) {
        std::cout << i << " |--xxHash--> " << xxHash(i) << std::endl;
    }

    return 0;
}
