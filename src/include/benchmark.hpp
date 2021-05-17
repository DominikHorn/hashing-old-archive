#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <vector>

#ifdef __APPLE__
   #include "TargetConditionals.h"
   #ifdef TARGET_OS_MAC
      #define MACOS
   #endif
#endif

#ifdef MACOS
   #warning "Detected MacOS, building with perf counter utility"
   #include <thirdparty/perf-macos.hpp>
#endif

namespace Benchmark {

   struct ThroughputStats {
      uint64_t average_total_inference_reduction_ns;
      unsigned int repeatCnt;
   };

   /**
    * measures throughput when hashing into a dataset.size() * over_alloc sized hashtable
    * using HashFunction to obtain a hash value and Reducer to reduce the hash value to an index into
    * the hashtable.
    *
    * Does not actually perform hashtable insert/lookup!
    *
    * @tparam Hashfn
    * @tparam Reducerfn
    */
   template<typename Hashfn, typename Reducefn, unsigned int repeatCnt = 10>
   ThroughputStats measure_throughput(const std::vector<uint64_t>& dataset) {
      const auto n = static_cast<uint64_t>(dataset.size());
      uint64_t avg = 0;

      Hashfn hashfn;
      Reducefn reducefn(n);

      for (unsigned int repetiton = 0; repetiton < repeatCnt; repetiton++) {
         const auto start_time = std::chrono::steady_clock::now();
         // Hash each value and record entries per bucket
#ifdef MACOS
         {
            Perf::BlockCounter ctr(dataset.size());
#endif
            for (const auto& key : dataset) {
               const auto index = reducefn(hashfn(key));

               // Ensure the compiler does not simply remove the index
               // calculation during optimization.
               Optimizer::DoNotEliminate(index);
            }
#ifdef MACOS
         }
#endif
         const auto end_time = std::chrono::steady_clock::now();
         const auto delta_ns =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());

         // we will lose at most one nanosecond precision each time which should
         // not make a difference in practice
         avg += delta_ns / repeatCnt;
      }

      return {avg, repeatCnt};
   }

   // These are probably to large but these few additional bytes don't hurt
   template<typename Counter, typename PreciseMath>
   struct CollisionStats {
      Counter min;
      Counter max;
      Counter empty_slots;
      Counter colliding_slots;
      Counter total_colliding_keys;
      PreciseMath std_dev;

      Counter inference_reduction_memaccess_total_ns;

      explicit CollisionStats(Counter inference_reduction_memaccess_total_ns)
         : min(0xFFFFFFFFFFFFFFFFLLU), max(0), empty_slots(0), colliding_slots(0), total_colliding_keys(0), std_dev(0),
           inference_reduction_memaccess_total_ns(inference_reduction_memaccess_total_ns) {}
   } __attribute__((aligned(64)));

   /**
    * Returns min, max and sum of elements hashed into a dataset.size() * over_alloc sized hashtable
    * using HashFunction to obtain a hash value and Reducer to reduce the hash value to an index into
    * the hashtable.
    *
    * @tparam HashFunction
    * @tparam Reducer
    */
   template<typename HashFunction, typename Reducer>
   CollisionStats<uint64_t, double> measure_collisions(const std::vector<uint64_t>& dataset,
                                                       std::vector<uint32_t>& collision_counter,
                                                       const HashFunction& hashfn, const Reducer& reduce) {
      // Emulate hashtable with slots (we only care about amount of elements per bucket)
      const auto n = collision_counter.size();
      std::fill(collision_counter.begin(), collision_counter.end(), 0);

      auto start_time = std::chrono::steady_clock::now();

#ifdef MACOS
      {
         Perf::BlockCounter ctr(dataset.size());
#endif
         // Hash each value and record entries per bucket
         for (const auto key : dataset) {
            const auto hash = hashfn(key);
            const auto index = reduce(hash, n);
            collision_counter[index]++;

            // Our datasets are too small to ever cause unsigned int addition overflow
            // therefore this check is redundant
            //         assert(collision_counter[index] != 0);
         }
#ifdef MACOS
      }
#endif
      auto end_time = std::chrono::steady_clock::now();
      uint64_t inference_reduction_memaccess_total_ns =
         static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());

      // Min has to start at max value for its type
      CollisionStats<uint64_t, double> stats(inference_reduction_memaccess_total_ns);
      double std_dev_square_sum = 0.0;
      const auto average =
         static_cast<long double>(dataset.size()) / static_cast<long double>(collision_counter.size());
      for (const auto bucket_cnt : collision_counter) {
         stats.min = std::min(static_cast<uint64_t>(bucket_cnt), stats.min);
         stats.max = std::max(static_cast<uint64_t>(bucket_cnt), stats.max);
         stats.empty_slots += bucket_cnt == 0 ? 1 : 0;
         stats.colliding_slots += bucket_cnt > 1 ? 1 : 0;
         stats.total_colliding_keys += bucket_cnt > 1 ? bucket_cnt : 0;
         std_dev_square_sum += (bucket_cnt - average) * (bucket_cnt - average);
      }
      stats.std_dev = std::sqrt(std_dev_square_sum / static_cast<double>(dataset.size()));

      return stats;
   }

   struct HashtableStats {
      uint64_t total_insert_ns;
      uint64_t total_lookup_ns;
   };

   template<typename Hashtable>
   HashtableStats measure_hashtable(const std::vector<typename Hashtable::KeyType>& dataset, Hashtable& ht) {
      // Ensure hashtable is empty when we begin
      ht.clear();

      // Insert every key
      auto start_time = std::chrono::steady_clock::now();
#ifdef MACOS
      {
         Perf::BlockCounter ctr(dataset.size());
#endif
         for (const auto key : dataset) {
            ht.insert(key, static_cast<typename Hashtable::PayloadType>(key) - 5);
         }
#ifdef MACOS
      }
#endif
      auto end_time = std::chrono::steady_clock::now();
      uint64_t total_insert_ns =
         static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());

      // Lookup every key
      start_time = std::chrono::steady_clock::now();
#ifdef MACOS
      {
         Perf::BlockCounter ctr(dataset.size());
#endif
         for (const auto& key : dataset) {
            const auto payload = ht.lookup(key);
            Optimizer::DoNotEliminate(payload);
            full_mem_barrier; // emulate doing something with payload by stalling at least until it arrives
            assert(payload);
            assert(payload.value() == static_cast<typename Hashtable::PayloadType>(key) - 5);
         }
#ifdef MACOS
      }
#endif
      end_time = std::chrono::steady_clock::now();
      uint64_t total_lookup_ns =
         static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());

      return {total_insert_ns, total_lookup_ns};
   }
} // namespace Benchmark
