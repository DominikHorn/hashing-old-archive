#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

// TODO: use proper benchmarking
#include <chrono>

#include "../convenience/convenience.hpp"
#include "args.hpp"

namespace Benchmark {
   // These are probably to large but these few additional bytes don't hurt
   template<typename Counter, typename PreciseMath>
   struct CollisionStats {
      Counter min;
      Counter max;
      Counter empty_buckets;
      Counter colliding_buckets;
      Counter total_collisions;
      PreciseMath std_dev;

      double inference_nanoseconds;

      CollisionStats(double inference_nanoseconds)
         : min(0xFFFFFFFFFFFFFFFFllu), max(0), empty_buckets(0), colliding_buckets(0), total_collisions(0), std_dev(0),
           inference_nanoseconds(inference_nanoseconds) {}
   };

   /**
    * Returns min, max and sum of elements hashed into a dataset.size() * over_alloc sized hashtable
    * using HashFunction to obtain a hash value and Reducer to reduce the hash value to an index into
    * the hashtable.
    *
    * @tparam HashFunction
    * @tparam Reducer
    */
   template<typename HashFunction, typename Reducer>
   CollisionStats<uint64_t, double> measure_collisions(const Args& args, const std::vector<uint64_t>& dataset,
                                                       const HashFunction& hashfn, const Reducer& reduce) {
      // Emulate hashtable with buckets (we only care about amount of elements per bucket)
      const auto n = (uint64_t) std::ceil(dataset.size() * args.over_alloc);
      std::vector<uint32_t> collision_counter(n, 0);

      auto start_time = std::chrono::steady_clock::now();
      // Hash each value and record entries per bucket
      for (const auto key : dataset) {
         const auto hash = hashfn(key);
         const auto index = reduce(hash, n);
         collision_counter[index]++;

         // Our datasets are too small to ever cause unsigned int addition overflow
         // therefore this check is redundant
         //         assert(collision_counter[index] != 0);
      }
      auto end_time = std::chrono::steady_clock::now();
      double inference_nanoseconds =
         std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

      // Min has to start at max value for its type
      CollisionStats<uint64_t, double> stats(inference_nanoseconds);
      double std_dev_square_sum = 0.0;
      const double average = 1.0 / args.over_alloc;

      for (const auto bucket_cnt : collision_counter) {
         stats.min = std::min((uint64_t) bucket_cnt, stats.min);
         stats.max = std::max((uint64_t) bucket_cnt, stats.max);
         stats.empty_buckets += bucket_cnt == 0 ? 1 : 0;
         stats.colliding_buckets += bucket_cnt > 1 ? 1 : 0; // TODO: think about how to make this branchless (for fun)
         stats.total_collisions += bucket_cnt > 1 ? bucket_cnt : 0;
         std_dev_square_sum += (bucket_cnt - average) * (bucket_cnt - average);
      }
      stats.std_dev = std::sqrt(std_dev_square_sum / (double) dataset.size());

      return stats;
   }
} // namespace Benchmark