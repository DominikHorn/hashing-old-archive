#include <fstream>
#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/dataset.hpp"

int main(const int argc, const char* argv[]) {
   std::ofstream outfile;

   try {
      // TODO: (?) automatically iterate over over_allocation parameter
      // TODO: log over_allocation in result csv
      auto args = Args::parse(argc, argv);
      outfile.open(args.outfile);
      outfile << "hash,min,max,std_dev,total_collisions,dataset" << std::endl;

      for (auto const& it : DATASETS) {
         // TODO: tmp (for debugging, remove for actually benchmarking)
         std::cout << "benchmarking " << it.first << std::endl;
         if (/*it.first != "wiki" &&*/ it.first != "debug")
            continue;

         const auto dataset = it.second.load(args.datapath);
         const auto measure = [&](std::string method, auto hashfn) {
            uint64_t min = 0;
            uint64_t max = 0;
            uint64_t total_collisions = 0;
            double std_dev = 0;

            std::tie(min, max, std_dev, total_collisions) =
               Benchmark::measure_collisions(args, dataset, hashfn, HashReduction::modulo<HASH_64>);

            outfile << method << "," << min << "," << max << "," << std_dev << "," << total_collisions << ","
                    << it.first << std::endl;
         };

         // More significant bits supposedly are of higher quality for multiplicative methods -> compute
         // how much we need to shift to throw away as few "high quality" bits as possible
         const auto hashtable_size = dataset.size() * args.over_alloc;
         const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);

         measure("mult64", [](HASH_64 key) { return MultHash::mult64_hash(key); });
         measure("mult64_shift", [p](HASH_64 key) { return MultHash::mult64_hash(key, p); });
         measure("fibo64", [](HASH_64 key) { return MultHash::fibonacci64_hash(key); });
         measure("fibo64_shift", [p](HASH_64 key) { return MultHash::fibonacci64_hash(key, p); });
         measure("fibo_prime64", [](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key); });
         measure("fibo_prime64_shift", [p](HASH_64 key) { return MultHash::fibonacci_prime64_hash(key, p); });
         measure("multadd64", [](HASH_64 key) { return MultAddHash::multadd64_hash(key); });
         measure("multadd64_shift", [p](HASH_64 key) { return MultAddHash::multadd64_hash(key, p); });
         measure("murmur3_128_low",
                 [](HASH_64 key) { return HashReduction::lower_half(MurmurHash3::murmur3_128(key)); });
         measure("murmur3_128_upp",
                 [](HASH_64 key) { return HashReduction::upper_half(MurmurHash3::murmur3_128(key)); });
         measure("murmur3_128_xor", [](HASH_64 key) { return HashReduction::xor_both(MurmurHash3::murmur3_128(key)); });
         measure("murmur3_fin64", [](HASH_64 key) { return MurmurHash3::finalize_64(key); });
      }

   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      outfile.close();
      return -1;
   }

   outfile.close();
   return 0;
}