#include <iostream>
#include <vector>

#include "xxHash.hpp"

int main() {
    // TODO: temporary test
    for (unsigned long long i = 0; i < 32; i++) {
        std::cout << i << " |--XXH64--> " << XXH64_hash(i) << std::endl;
        std::cout << i << " |--XXH3---> " << XXH3_hash(i) << std::endl;
    }

    return 0;
}
