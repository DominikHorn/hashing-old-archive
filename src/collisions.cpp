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

         measure("mult64", [](HASH_64 key) { return MultHash::mult64_hash(key); });
         measure("fibo64", [](HASH_64 key) { return MultHash::fibonacci64_hash(key); });
      }

   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      outfile.close();
      return -1;
   }

   outfile.close();
   return 0;
}