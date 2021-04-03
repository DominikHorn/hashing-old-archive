#include <iostream>
#include <string>
#include <vector>

#include "args.hpp"
#include "hashing.hpp"

// TODO: murmur (-> widely used baseline)
// TODO: tabulation hashing (-> robustness benchmarks)
// TODO: cityhash (-> widely used baseline)
// TODO: aquahash (-> fast)
// TODO: farmhash (-> fast(er) than cityhash)
// TODO: spookyhash (-> not well known murmur competitor)

int main(const int argc, const char* argv[]) {
   try {
      auto args = Args::parse(argc, argv);

      // TODO: tmp (test)
      for (uint64_t i = 0; i < 32; i++) {
         const auto val = args.dataset[i];
         std::cout << val << " |--XXH64--> " << XXHash::XXH64_hash(val) << std::endl;
         std::cout << val << " |--XXH3---> " << XXHash::XXH3_hash(val) << std::endl;
         std::cout << val << " |--mult32---> " << MultHash::mult32_hash(val) << std::endl;
         std::cout << val << " |--mult64--> " << MultHash::mult64_hash(val) << std::endl;
         std::cout << val << " |--fibo32---> " << MultHash::fibonacci32_hash(val) << std::endl;
         std::cout << val << " |--fibo64---> " << MultHash::fibonacci64_hash(val) << std::endl;
         std::cout << val << " |--fiboPrime32--> " << MultHash::fibonacci_prime32_hash(val) << std::endl;
         std::cout << val << " |--fiboPrime64--> " << MultHash::fibonacci_prime64_hash(val) << std::endl;
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
