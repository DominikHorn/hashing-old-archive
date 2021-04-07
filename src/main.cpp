#include <iostream>
#include <string>

#include "args.hpp"
#include "benchmark.hpp"
#include "hashing.hpp"

//// TODO: tmp test code
//void debug_tmp(const Args& args) {
//   //    Prepare a tabulation hash table
//   HASH_64 table[sizeof(HASH_64)][0xFF] = {0};
//   TabulationHash::gen_table(table);
//
//   for (uint64_t i = 0; i < 32; i++) {
//      const auto val = args.dataset[i];
//      std::cout << std::endl;
//      std::cout << val << " |--XXH64---------> " << XXHash::XXH64_hash(val) << std::endl;
//      std::cout << val << " |--XXH3----------> " << XXHash::XXH3_hash(val) << std::endl;
//      std::cout << val << " |--XXH3_128_low--> " << HashReduction::lower_half(XXHash::XXH3_128_hash(val)) << std::endl;
//      std::cout << val << " |--XXH3_128_upp--> " << HashReduction::upper_half(XXHash::XXH3_128_hash(val)) << std::endl;
//      std::cout << val << " |--XXH3_128_xor--> " << HashReduction::xor_both(XXHash::XXH3_128_hash(val)) << std::endl;
//
//      std::cout << std::endl;
//      std::cout << val << " |--mult32--------> " << MultHash::mult32_hash(val) << std::endl;
//      std::cout << val << " |--mult64--------> " << MultHash::mult64_hash(val) << std::endl;
//      std::cout << val << " |--fibo32--------> " << MultHash::fibonacci32_hash(val) << std::endl;
//      std::cout << val << " |--fibo64--------> " << MultHash::fibonacci64_hash(val) << std::endl;
//      std::cout << val << " |--fiboPrime32---> " << MultHash::fibonacci_prime32_hash(val) << std::endl;
//      std::cout << val << " |--fiboPrime64---> " << MultHash::fibonacci_prime64_hash(val) << std::endl;
//
//      std::cout << std::endl;
//      std::cout << val << " |--multadd32-----> " << MultAddHash::multadd32_hash(val) << std::endl;
//      std::cout << val << " |--multadd64-----> " << MultAddHash::multadd64_hash(val) << std::endl;
//
//      std::cout << std::endl;
//      std::cout << val << " |--murmur32_fin--> " << MurmurHash3::finalize_32(val) << std::endl;
//      std::cout << val << " |--murmur64_fin--> " << MurmurHash3::finalize_64(val) << std::endl;
//      std::cout << val << " |--murmur32------> " << MurmurHash3::murmur3_32(val) << std::endl;
//      std::cout << val << " |--murmur128_low-> " << HashReduction::lower_half(MurmurHash3::murmur3_128(val))
//                << std::endl;
//      std::cout << val << " |--murmur128_upp-> " << HashReduction::upper_half(MurmurHash3::murmur3_128(val))
//                << std::endl;
//      std::cout << val << " |--murmur128_xor-> " << HashReduction::xor_both(MurmurHash3::murmur3_128(val)) << std::endl;
//
//      std::cout << std::endl;
//      std::cout << val << " |--tabulation----> " << TabulationHash::naive_hash(i, table) << std::endl;
//   }
//}

int main(const int argc, const char* argv[]) {
   try {
      auto args = Args::parse(argc, argv);

      uint32_t min = 0;
      uint32_t max = 0;
      std::vector<uint32_t> stats = {};

      std::tie(min, max, stats) = Benchmark::measure_collisions(
         args,
         [](HASH_64 key) { return MultHash::mult64_hash(key); },
         HashReduction::modulo<HASH_64>);
      std::cout << "mult64_hash (min: " << min << ", max: " << max << ")" << std::endl;

      std::tie(min, max, stats) = Benchmark::measure_collisions(
         args,
         [](HASH_64 key) { return MultHash::fibonacci64_hash(key); },
         HashReduction::modulo<HASH_64>);
      std::cout << "fibonacci64_hash (min: " << min << ", max: " << max << ")" << std::endl;
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
