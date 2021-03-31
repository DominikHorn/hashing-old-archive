#include <iostream>
#include <string>
#include <vector>

#include "args.hpp"
#include "hashing.hpp"

// TODO: multiplicative hashing (standard,mult+add (theoretical properties + good with native 128 bit arithmetic!),fibbonacci,etc) (-> fast)
// TODO: murmur (-> widely used baseline)
// TODO: tabulation hashing (-> robustness benchmarks)
// TODO: cityhash (-> widely used baseline)
// TODO: aquahash (-> fast)
// TODO: farmhash (-> fast(er) than cityhash)
// TODO: spookyhash (-> murmur competitor)

int main(const int argc, const char* argv[]) {
   try {
      auto args = Args::parse(argc, argv);
   } catch (std::string msg) {
      std::cerr << msg << std::endl;
      exit(-1);
   }

   // TODO: tmp (test)
   //   for (unsigned long long i = 0; i < 32; i++) {
   //      std::cout << i << " |--XXH64--> " << XXH64_hash(i) << std::endl;
   //      std::cout << i << " |--XXH3---> " << XXH3_hash(i) << std::endl;
   //   }

   return 0;
}
