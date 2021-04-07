#include <algorithm>
#include <cmath>
#include <vector>

#include "../convenience/convenience.hpp"
#include "args.hpp"

namespace Benchmark {
   /**
    * Returns min, max and sum of elements hashed into a dataset.size() * over_alloc sized hashtable
    * using HashFunction to obtain a hash value and Reducer to reduce the hash value to an index into
    * the hashtable.
    *
    * @tparam HashFunction
    * @tparam Reducer
    * @tparam Counter
    */
   template<typename HashFunction, typename Reducer, typename Counter = uint32_t>
   neverinline std::tuple<Counter, Counter, Counter> measure_collisions(const Args& args, const HashFunction hash,
                                                                        const Reducer reduce) {
      // Emulate hashtable with buckets (we only care about amount of elements per bucket)
      const auto n = (uint64_t) std::ceil(args.dataset.size() * args.over_alloc);
      std::vector<Counter> collision_counter(n, 0);

      // Hash each value and record entries per bucket
      for (const auto key : args.dataset) {
         collision_counter[reduce(hash(key), n)]++;
      }

      // Min has to start at max value for its type
      Counter min = 0;
      for (auto i = 0; i < sizeof(Counter); i++) {
         min |= ((Counter) 0xFF) << i * 8;
      }
      Counter max = 0;
      Counter sum = 0;

      for (const auto bucketCnt : collision_counter) {
         min = std::min(bucketCnt, min);
         max = std::max(bucketCnt, max);
         sum += bucketCnt;
      }

      return {min, max, sum};
   }
} // namespace Benchmark