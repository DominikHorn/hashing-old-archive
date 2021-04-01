#include <iostream>
#include <string>
#include <vector>

#include "args.hpp"
#include "hashing.hpp"

// TODO: mult+add hashing (theoretical good properties + excellent iff cpu has native 128 bit arithmetic)
// TODO: murmur (-> widely used baseline)
// TODO: tabulation hashing (-> robustness benchmarks)
// TODO: cityhash (-> widely used baseline)
// TODO: aquahash (-> fast)
// TODO: farmhash (-> fast(er) than cityhash)
// TODO: spookyhash (-> murmur competitor)

int main(const int argc, const char* argv[]) {
   try {
      auto args = Args::parse(argc, argv);

      // TODO: tmp (test)
      for (uint64_t i = 0; i < 32; i++) {
         const auto val = args.dataset[i];
         std::cout << val << " |--XXH64--> " << XXHash::XXH64_hash(val) << std::endl;
         std::cout << val << " |--XXH3---> " << XXHash::XXH3_hash(val) << std::endl;
         std::cout << val << " |--mult---> " << MultHash::mult64_hash(val) << std::endl;
         std::cout << val << " |--fibo---> " << MultHash::fibonacci64_hash(val) << std::endl;
         std::cout << val << " |--fiboPrime--> " << MultHash::fibonacci_prime64_hash(val) << std::endl;
      }
   } catch (std::string msg) {
      std::cerr << msg << std::endl;
      return -1;
   }

   return 0;
}
