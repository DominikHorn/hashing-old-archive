#include "hashing.hpp"
#include "include/args.hpp"

int main(const int argc, const char* argv[]) {
   auto args = Args::parse(argc, argv);

   for (const auto& it : args.datasets) {
      for (auto i : it.load_shuffled(args.datapath)) {
         std::cout << i << ", ";
      }
      std::cout << std::endl;

      for (auto i : it.load(args.datapath)) {
         std::cout << i << ", ";
      }
      std::cout << std::endl;
   }
}