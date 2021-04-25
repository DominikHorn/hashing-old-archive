#include <functional>

#include <convenience.hpp>
#include <hashing.hpp>
#include <hashtable.hpp>

int main(int argc, char* argv[]) {
   const auto zero_hash = [](const HASH_64& val, const size_t& N) { return 0; };

   {
      Hashtable::Chained<HASH_64, uint32_t, 4> chained(10);

      for (size_t r = 0; r < 10; r++) {
         for (size_t i = 0; i < 10000; i++) {
            auto key = rand();
            chained.insert(key, 0xFF, zero_hash);
            assert(chained.lookup(key, zero_hash) == 0xFF);
         }

         chained.clear();
      }
      std::cout << "Test: " << chained.size() << std::endl;
   }
   return 0;
}