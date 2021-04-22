#include <functional>

#include <convenience.hpp>
#include <hashing.hpp>
#include <hashtable.hpp>
#include <reduction.hpp>

struct mult64_fastrange64 {
   static forceinline size_t hash(const HASH_64& key, const size_t& N) {
      const auto hash = MultHash::mult64_hash(key);
      const auto index = Reduction::fastrange(hash, N);
      return index;
   }
};

int main(int argc, char* argv[]) {
   Hashtable::Chained<HASH_64, uint64_t, mult64_fastrange64> chained(100000000);

   std::vector<HASH_64> keys(100, 0);
   std::cout << "keys: ";
   for (HASH_64& key : keys) {
      key = rand();
      std::cout << key << ", ";
   }
   std::cout << std::endl;

   for (auto key : keys) {
      const auto payload = key - 5;
      chained.insert(key, payload);
      assert(chained.lookup(key) == payload);
   }

   for (auto key : keys) {
      assert(chained.lookup(key) == key - 5);
   }

   std::cout << "Test: " << chained.size() << std::endl;
}