#include "include/csv.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
   CSV csv("../../debug.csv",
           {"dataset", "numelements", "load_factor", "sample_size", "bucket_size", "model", "reducer", "sample_size",
            "insert_nanoseconds_total", "insert_nanoseconds_per_key", "lookup_nanoseconds_total",
            "lookup_nanoseconds_per_key"});

   const auto str = [](auto s) { return std::to_string(s); };
   std::cout << csv.exists({{"dataset", "debug_64"}}) << std::endl;
   std::cout << csv.exists({{"dataset", "debug_62"}}) << std::endl;
   std::cout << csv.exists({{"load_factor", str(0.25)}, {"dataset", "debug_64"}}) << std::endl;
   std::cout << csv.exists({{"model", "pgm_hash_eps64"}}) << std::endl;
   std::cout << csv.exists({{"dataset", "debug_64"}, {"model", "pgm_hash_eps64"}, {"load_factor", str(0.25)}})
             << std::endl;
   std::cout << csv.exists({{"model", "pgm_hash_eps64"}, {"dataset", "debug_64"}}) << std::endl;
   std::cout << csv.exists({{"model", "pgm_hash_eps64"}, {"dataset", "debug_64 "}}) << std::endl;

   return 0;
}