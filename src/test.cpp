#include <cstdint>

#include "hashing.hpp"

int main(int argc, char* argv[]) {
   // Prepare a tabulation hash table
   HASH_64 table[sizeof(HASH_64)][0xFF] = {0};
   TabulationHash::gen_table(table);

   for (uint64_t i = 0; i < 256; i++) {
      std::cout << std::endl;
      std::cout << i << " |--XXH64---------> " << XXHash::XXH64_hash(i) << std::endl;
      std::cout << i << " |--XXH3----------> " << XXHash::XXH3_hash(i) << std::endl;
      std::cout << i << " |--XXH3_128_low--> " << HashReduction::lower_half(XXHash::XXH3_128_hash(i)) << std::endl;
      std::cout << i << " |--XXH3_128_upp--> " << HashReduction::upper_half(XXHash::XXH3_128_hash(i)) << std::endl;
      std::cout << i << " |--XXH3_128_xor--> " << HashReduction::xor_both(XXHash::XXH3_128_hash(i)) << std::endl;

      std::cout << std::endl;
      std::cout << i << " |--mult32--------> " << MultHash::mult32_hash(i) << std::endl;
      std::cout << i << " |--mult64--------> " << MultHash::mult64_hash(i) << std::endl;
      std::cout << i << " |--fibo32--------> " << MultHash::fibonacci32_hash(i) << std::endl;
      std::cout << i << " |--fibo64--------> " << MultHash::fibonacci64_hash(i) << std::endl;
      std::cout << i << " |--fiboPrime32---> " << MultHash::fibonacci_prime32_hash(i) << std::endl;
      std::cout << i << " |--fiboPrime64---> " << MultHash::fibonacci_prime64_hash(i) << std::endl;

      std::cout << std::endl;
      std::cout << i << " |--multadd32-----> " << MultAddHash::multadd32_hash(i) << std::endl;
      std::cout << i << " |--multadd64-----> " << MultAddHash::multadd64_hash(i) << std::endl;

      std::cout << std::endl;
      std::cout << i << " |--murmur32_fin--> " << MurmurHash3::finalize_32(i) << std::endl;
      std::cout << i << " |--murmur64_fin--> " << MurmurHash3::finalize_64(i) << std::endl;
      std::cout << i << " |--murmur32------> " << MurmurHash3::murmur3_32(i) << std::endl;
      std::cout << i << " |--murmur128_low-> " << HashReduction::lower_half(MurmurHash3::murmur3_128(i)) << std::endl;
      std::cout << i << " |--murmur128_upp-> " << HashReduction::upper_half(MurmurHash3::murmur3_128(i)) << std::endl;
      std::cout << i << " |--murmur128_xor-> " << HashReduction::xor_both(MurmurHash3::murmur3_128(i)) << std::endl;

      std::cout << std::endl;
      std::cout << i << " |--tabulation----> " << TabulationHash::naive_hash(i, table) << std::endl;
   }
}