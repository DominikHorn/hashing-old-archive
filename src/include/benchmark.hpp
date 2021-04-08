#include <algorithm>
#include <cassert>
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
    */
   template<typename HashFunction, typename Reducer>
   std::tuple<uint64_t, uint64_t, double, uint64_t, uint64_t, uint64_t> // TODO: return struct instead of tuple
   measure_collisions(const Args& args, const std::vector<uint64_t>& dataset, const HashFunction hashfn,
                      const Reducer reduce) {
      // Emulate hashtable with buckets (we only care about amount of elements per bucket)
      const auto n = (uint64_t) std::ceil(dataset.size() * args.over_alloc);
      std::vector<uint32_t> collision_counter(n, 0);

      // Hash each value and record entries per bucket
      for (const auto key : dataset) {
         const auto hash = hashfn(key);
         const auto index = reduce(hash, n);
         collision_counter[index]++;

         // Our datasets are too small to ever cause unsigned int addition overflow
         // therefore this check is redundant
         //         assert(collision_counter[index] != 0);
      }

      // Min has to start at max value for its type
      uint64_t min = 0xFFFFFFFFFFFFFFFF;
      uint64_t max = 0;
      uint64_t empty_buckets = 0;
      uint64_t colliding_buckets = 0;
      uint64_t total_collisions = 0;
      double std_dev_square = 0.0;
      const double average = 1.0 / args.over_alloc;

      for (const auto bucket_cnt : collision_counter) {
         min = std::min((uint64_t) bucket_cnt, min);
         max = std::max((uint64_t) bucket_cnt, max);
         empty_buckets += bucket_cnt == 0 ? 1 : 0;
         colliding_buckets += bucket_cnt > 1 ? 1 : 0; // TODO: think about how to make this branchless (for fun)
         total_collisions += bucket_cnt > 1 ? bucket_cnt : 0;
         std_dev_square += (bucket_cnt - average) * (bucket_cnt - average);
      }
      double std_dev = std::sqrt(std_dev_square / (double) dataset.size());

      return {min, max, std_dev, empty_buckets, colliding_buckets, total_collisions};
   }
} // namespace Benchmark