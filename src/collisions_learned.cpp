#define VERBOSE

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <learned_models.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/random_hash.hpp"

using Args = BenchmarkArgs::LearnedCollisionArgs;

// TODO: rework
const std::vector<std::string> csv_columns = {
   "dataset",
   "numelements",
   "load_factor",
   "hash",
   "reducer",
   "min",
   "max",
   "std_dev",
   "empty_slots",
   "empty_slots_percent",
   "colliding_slots",
   "colliding_slots_percent",
   "total_colliding_keys",
   "total_colliding_keys_percent",
   "nanoseconds_total",
   "nanoseconds_per_key",
};

static void measure(const std::string& dataset_name, const std::vector<uint64_t>& dataset, const double load_factor,
                    CSV& outfile, std::mutex& iomutex) {
   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset.size()) / static_cast<double>(load_factor));

   // lambda to measure a given hash function with a given reducer. Will be entirely inlined by default
   std::vector<uint32_t> collision_counter(hashtable_size);
   const auto measure_hashfn_with_reducer = [&](const std::string& hash_name, const auto& hashfn,
                                                const std::string& reducer_name, const auto& reducerfn) {
      // Prepare & measure
      std::fill(collision_counter.begin(), collision_counter.end(), 0);
      const auto stats = Benchmark::measure_collisions(dataset, collision_counter, hashfn, reducerfn);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") took "
                   << relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.inference_reduction_memaccess_total_ns) << " s total)" << std::endl;
      };
#endif

      const auto str = [](auto s) { return std::to_string(s); };
      outfile.write({
         {"dataset", dataset_name},
         {"numelements", str(dataset.size())},
         {"load_factor", str(load_factor)},
         {"hash", hash_name},
         {"reducer", reducer_name},
         {"min", str(stats.min)},
         {"max", str(stats.max)},
         {"std_dev", str(stats.std_dev)},
         {"empty_slots", str(stats.empty_slots)},
         {"empty_slots_percent", str(relative_to(stats.empty_slots, hashtable_size))},
         {"colliding_slots", str(stats.colliding_slots)},
         {"colliding_slots_percent", str(relative_to(stats.colliding_slots, hashtable_size))},
         {"total_colliding_keys", str(stats.total_colliding_keys)},
         {"total_colliding_keys_percent", str(relative_to(stats.total_colliding_keys, hashtable_size))},
         {"nanoseconds_total", str(stats.inference_reduction_memaccess_total_ns)},
         {"nanoseconds_per_key", str(relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size()))},
      });
   };

   /**
    * ====================================
    *               Baseline
    * ====================================
    */

   /**
    * ====================================
    *       Learned hash functions
    * ====================================
    */
   {
      // TODO: move this logic to separate file (benchmark.hpp?)

      // Take a random sample
      auto start_time = std::chrono::steady_clock::now();
      // TODO: make sample size a cli argument. Switch to cxxopts for cli parsing (overload << ?)
      const auto sample_size = static_cast<size_t>(0.01 * dataset.size());
      std::vector<uint64_t> sample(sample_size, 0);

      const uint64_t seed = 0x9E3779B9LU; // Random constant to ensure reproducibility
      std::default_random_engine gen(seed);
      std::uniform_int_distribution<uint64_t> dist(0, dataset.size() - 1);
      for (size_t i = 0; i < sample_size; i++) {
         const auto random_index = dist(gen);
         sample[i] = dataset[random_index];
      }
      uint64_t sample_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      // Sort the sample
      start_time = std::chrono::steady_clock::now();
      std::sort(sample.begin(), sample.end());
      uint64_t sort_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      start_time = std::chrono::steady_clock::now();
      // TODO: benchmark different epsilon values
      const int epsilon = 128;
      pgm::PGMIndex<uint64_t, epsilon> pgm(sample);
      uint64_t build_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      // TODO: write this extra information to csv file
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "pgm (sample: " << sample_ns << " ns, sort: " << sort_ns << " ns, build: " << build_ns << " ns)"
                   << std::endl;
      };

      // TODO: move this to hash reduction.hpp
      const auto min_max_cutoff = [](const auto& pos, const auto& N) {
         return std::max(static_cast<size_t>(0), std::min(pos, N - 1));
      };

      // TODO: test different reduction methods, i.e., check if out of bounds with
      //  unlikely() annotation and if so, apply standard reducer?
      measure_hashfn_with_reducer(
         "pgm", [&](const HASH_64& key) { return pgm.search(key).pos; }, "min_max_cutoff", min_max_cutoff);
   }
}

int main(int argc, char* argv[]) {
   // Worker pool for speeding up the benchmarking. We never run more threads than
   // available CPUs to ensure that results are accurate!
   // TODO: implement num_cpus argument that overwrites this value
   const auto num_cpus = std::thread::hardware_concurrency();
   std::cout << "Detected " << num_cpus << " available cpus" << std::endl;

   std::mutex iomutex;
   std::counting_semaphore cpu_blocker(num_cpus);
   std::vector<std::thread> threads{};

   try {
      auto args = Args(argc, argv);
      CSV outfile(args.outfile, csv_columns);

      for (const auto& it : args.datasets) {
         // TODO: once we have more RAM we maybe should load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. Also look into setting CPU affinity. I.e.
         //  move this load into threads after aquire()
         const std::vector<uint64_t> dataset = it.load();

         for (auto load_factor : args.load_factors) {
            threads.emplace_back(std::thread([&, load_factor] {
               cpu_blocker.aquire();
               measure(it.name(), dataset, load_factor, outfile, iomutex);
               cpu_blocker.release();
            }));
         }

         // TODO: to increase parallelization degree, move this await/join() one scope up.
         //  The semaphore already prevents executing more threads than available cpus.
         //  The only reason to keep the join() here is to limit ram usage
         for (auto& t : threads) {
            t.join();
         }
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}