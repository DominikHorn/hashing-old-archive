#include <iostream>
#include <string>

#include "hashing.hpp"

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/dataset.hpp"

int main(const int argc, const char* argv[]) {
   try {
      auto args = Args::parse(argc, argv);

      for (auto const& it : DATASETS) {
         std::cout << "benchmarking " << it.first << std::endl;

         // TODO: tmp (prevent overload)
         if (it.first != "wiki" && it.first != "debug")
            continue;

         const auto dataset = it.second.load(args.datapath);

         uint32_t min = 0;
         uint32_t max = 0;
         std::vector<uint32_t> stats = {};

         std::tie(min, max, stats) = Benchmark::measure_collisions(
            args,
            dataset,
            [](HASH_64 key) { return MultHash::mult64_hash(key); },
            HashReduction::modulo<HASH_64>);
         std::cout << "mult64_hash (min: " << min << ", max: " << max << ")" << std::endl;

         std::tie(min, max, stats) = Benchmark::measure_collisions(
            args,
            dataset,
            [](HASH_64 key) { return MultHash::fibonacci64_hash(key); },
            HashReduction::modulo<HASH_64>);
         std::cout << "fibonacci64_hash (min: " << min << ", max: " << max << ")" << std::endl;
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}