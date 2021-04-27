#include <iostream>
#include <vector>

#include <hashing.hpp>
#include <hashtable.hpp>
#include <reduction.hpp>

int main(int argc, char* argv[]) {
   const size_t N = 10000;
   const double load_fac = 0.99;

   Hashtable::Cuckoo<uint32_t, uint32_t, 4> ht(N);

   for (uint32_t key = 1000; key < (N - 1000) * load_fac; key++) {
      ht.insert(
         key, key + 1, MurmurHash3::finalize_32,
         [](const auto& key, const auto& h1) { return MurmurHash3::finalize_32(h1 ^ key); },
         Reduction::fastrange<uint32_t>);
   }

   for (uint32_t key = 1000; key < (N - 1000) * load_fac; key++) {
      const auto value = ht.lookup(
         key,
         MurmurHash3::finalize_32,
         [](const auto& key, const auto& h1) { return MurmurHash3::finalize_32(h1 ^ key); },
         Reduction::fastrange<uint32_t>);

      if (!value.has_value() || key + 1 != value.value()) {
         return 1;
      }
   }

   return 0;
}