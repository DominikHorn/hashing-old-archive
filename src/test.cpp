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
   Hashtable::Chained<HASH_64, uint64_t, mult64_fastrange64> chained(50);

   for (HASH_64 key = 0; key < 100; key++) {
      chained.insert(key, key - 5);
   }

   std::cout << "Test: " << chained.size() << std::endl;
}