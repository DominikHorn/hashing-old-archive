#include <convenience.hpp>
#include <hashing.hpp>

#include "include/args.hpp"

int main(const int argc, const char* argv[]) {
   HASH_64 small_tabulation_table[0xFF] = {0};
   HASH_64 large_tabulation_table[sizeof(HASH_64)][0xFF] = {0};
   TabulationHash::gen_column(small_tabulation_table);
   TabulationHash::gen_table(large_tabulation_table);

   TabulationHash::print_table(large_tabulation_table);
   TabulationHash::print_column(small_tabulation_table);

   //   auto args = Args::parse(argc, argv);
   //   for (const auto& it : args.datasets) {
   //      for (auto i : it.load_shuffled(args.datapath)) {
   //         std::cout << i << ", ";
   //      }
   //      std::cout << std::endl;
   //
   //      for (auto i : it.load(args.datapath)) {
   //         std::cout << i << ", ";
   //      }
   //      std::cout << std::endl;
   //   }
}