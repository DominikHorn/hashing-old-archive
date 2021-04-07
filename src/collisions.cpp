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
      auto args = Args::parse(argc, argv);
      outfile.open(args.outfile);
      outfile << "hash,min,max,std_dev,dataset" << std::endl;

      for (auto const& it : DATASETS) {
         // TODO: tmp (for debugging, remove for actually benchmarking)
         std::cout << "benchmarking " << it.first << std::endl;
         if (/*it.first != "wiki" &&*/ it.first != "debug")
            continue;

         const auto dataset = it.second.load(args.datapath);

         uint32_t min = 0;
         uint32_t max = 0;
         double std_dev = 0;

         std::tie(min, max, std_dev) = Benchmark::measure_collisions(
            args,
            dataset,
            [](HASH_64 key) { return MultHash::mult64_hash(key); },
            HashReduction::modulo<HASH_64>);
         outfile << "mult64," << min << "," << max << "," << std_dev << "," << it.first << std::endl;

         std::tie(min, max, std_dev) = Benchmark::measure_collisions(
            args,
            dataset,
            [](HASH_64 key) { return MultHash::fibonacci64_hash(key); },
            HashReduction::modulo<HASH_64>);
         outfile << "fibo64," << min << "," << max << "," << std_dev << "," << it.first << std::endl;
      }

   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      outfile.close();
      return -1;
   }

   outfile.close();
   return 0;
}