#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <convenience.hpp>
#include <hashing.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/random_hash.hpp"

using Args = BenchmarkArgs::HashCollisionArgs;

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

static void measure(const std::string& dataset_name, const std::shared_ptr<const std::vector<uint64_t>> dataset,
                    const double load_factor, CSV& outfile, std::mutex& iomutex,
                    const HASH_64 (&small_tabulation_table)[0xFF],
                    const HASH_64 (&large_tabulation_table)[sizeof(HASH_64)][0xFF]) {
   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset->size()) / static_cast<double>(load_factor));

   // Build load factor specific auxiliary data
   const auto magic_branchfree_div = Reduction::make_branchfree_magic_divider(static_cast<HASH_64>(hashtable_size));

   // lambda to measure a given hash function with a given reducer. Will be entirely inlined by default
   std::vector<uint32_t> collision_counter(hashtable_size);
   const auto measure_hashfn_with_reducer = [&](const std::string& hash_name, const auto& hashfn,
                                                const std::string& reducer_name, const auto& reducerfn) {
      const auto str = [](auto s) { return std::to_string(s); };
      std::map<std::string, std::string> datapoint({
         {"dataset", dataset_name},
         {"numelements", str(dataset->size())},
         {"load_factor", str(load_factor)},
         {"hash", hash_name},
         {"reducer", reducer_name},
      });

      if (outfile.exists(datapoint)) {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "Skipping (";
         auto iter = datapoint.begin();
         while (iter != datapoint.end()) {
            std::cout << iter->first << ": " << iter->second;

            iter++;
            if (iter != datapoint.end())
               std::cout << ", ";
         }
         std::cout << ") since it already exist" << std::endl;
         return;
      }

      // Measure
      const auto stats = Benchmark::measure_collisions(*dataset, collision_counter, hashfn, reducerfn);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") took "
                   << relative_to(stats.inference_reduction_memaccess_total_ns, dataset->size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.inference_reduction_memaccess_total_ns) << " s total)" << std::endl;
      };
#endif

      datapoint.emplace("min", str(stats.min));
      datapoint.emplace("max", str(stats.max));
      datapoint.emplace("std_dev", str(stats.std_dev));
      datapoint.emplace("empty_slots", str(stats.empty_slots));
      datapoint.emplace("empty_slots_percent", str(relative_to(stats.empty_slots, hashtable_size)));
      datapoint.emplace("colliding_slots", str(stats.colliding_slots));
      datapoint.emplace("colliding_slots_percent", str(relative_to(stats.colliding_slots, hashtable_size)));
      datapoint.emplace("total_colliding_keys", str(stats.total_colliding_keys));
      datapoint.emplace("total_colliding_keys_percent", str(relative_to(stats.total_colliding_keys, dataset->size())));
      datapoint.emplace("nanoseconds_total", str(stats.inference_reduction_memaccess_total_ns));
      datapoint.emplace("nanoseconds_per_key",
                        str(relative_to(stats.inference_reduction_memaccess_total_ns, dataset->size())));

      // Write to csv
      outfile.write(datapoint);
   };

   // measures a hash function using every reducer
   const auto measure_hashfn = [&](const std::string& hash_name, const auto& hashfn) {
      measure_hashfn_with_reducer(hash_name, hashfn, "fastrange32", Reduction::fastrange<HASH_32>);
      measure_hashfn_with_reducer(hash_name, hashfn, "fastrange64", Reduction::fastrange<HASH_64>);

      // modulo, fast_modulo and branchless_fast_modulo only differ in the speed at which they complete computations
      measure_hashfn_with_reducer(hash_name, hashfn, "branchless_fast_modulo",
                                  [&magic_branchfree_div](const HASH_64& value, const HASH_64& n) {
                                     return Reduction::magic_modulo(value, n, magic_branchfree_div);
                                  });
   };

   /// Baseline
   {
      RandomHash<HASH_64> rand_uni(hashtable_size);
      measure_hashfn("random_uniform", [&](const HASH_64& key) { return rand_uni.next(); });
   }

   /**
    * ====================================
    *        Classic hash functions
    * ====================================
    */
   measure_hashfn("mult64", [](const HASH_64& key) { return MultHash::mult64_hash(key); });
   measure_hashfn("fibo64", [](const HASH_64& key) { return MultHash::fibonacci64_hash(key); });
   measure_hashfn("fibo_prime64", [](const HASH_64& key) { return MultHash::fibonacci_prime64_hash(key); });
   measure_hashfn("multadd64", [](const HASH_64& key) { return MultAddHash::multadd64_hash(key); });

   // More significant bits supposedly are of higher quality for multiplicative methods -> compute
   // how much we need to shift/rotate to throw away the least/make 'high quality bits' as prominent as possible
   const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);
   measure_hashfn("mult64_shift" + std::to_string(p),
                  [p](const HASH_64& key) { return MultHash::mult64_hash(key, p); });
   measure_hashfn("fibo64_shift" + std::to_string(p),
                  [p](const HASH_64& key) { return MultHash::fibonacci64_hash(key, p); });
   measure_hashfn("fibo_prime64_shift" + std::to_string(p),
                  [p](const HASH_64& key) { return MultHash::fibonacci_prime64_hash(key, p); });
   measure_hashfn("multadd64_shift" + std::to_string(p),
                  [p](const HASH_64& key) { return MultAddHash::multadd64_hash(key, p); });
   const unsigned int rot = 64 - p;
   measure_hashfn("mult64_rotate" + std::to_string(rot),
                  [&](const HASH_64& key) { return rotr(MultHash::mult64_hash(key), rot); });
   measure_hashfn("fibo64_rotate" + std::to_string(rot),
                  [&](const HASH_64& key) { return rotr(MultHash::fibonacci64_hash(key), rot); });
   measure_hashfn("fibo_prime64_rotate" + std::to_string(rot),
                  [&](const HASH_64& key) { return rotr(MultHash::fibonacci_prime64_hash(key), rot); });
   measure_hashfn("multadd64_rotate" + std::to_string(rot),
                  [&](const HASH_64& key) { return rotr(MultAddHash::multadd64_hash(key), rot); });

   measure_hashfn("murmur3_128_low",
                  [](const HASH_64& key) { return Reduction::lower_half(MurmurHash3::murmur3_128(key)); });
   measure_hashfn("murmur3_128_upp",
                  [](const HASH_64& key) { return Reduction::upper_half(MurmurHash3::murmur3_128(key)); });
   measure_hashfn("murmur3_128_xor",
                  [](const HASH_64& key) { return Reduction::xor_both(MurmurHash3::murmur3_128(key)); });
   measure_hashfn("murmur3_128_city",
                  [](const HASH_64& key) { return Reduction::hash_128_to_64(MurmurHash3::murmur3_128(key)); });
   measure_hashfn("murmur3_fin64", [](const HASH_64& key) { return MurmurHash3::finalize_64(key); });

   measure_hashfn("xxh64", [](const HASH_64& key) { return XXHash::XXH64_hash(key); });
   measure_hashfn("xxh3", [](const HASH_64& key) { return XXHash::XXH3_hash(key); });
   measure_hashfn("xxh3_128_low", [](const HASH_64& key) { return Reduction::lower_half(XXHash::XXH3_128_hash(key)); });
   measure_hashfn("xxh3_128_upp", [](const HASH_64& key) { return Reduction::upper_half(XXHash::XXH3_128_hash(key)); });
   measure_hashfn("xxh3_128_xor", [](const HASH_64& key) { return Reduction::xor_both(XXHash::XXH3_128_hash(key)); });
   measure_hashfn("xxh3_128_city",
                  [](const HASH_64& key) { return Reduction::hash_128_to_64(XXHash::XXH3_128_hash(key)); });

   measure_hashfn("tabulation_small64",
                  [&](const HASH_64& key) { return TabulationHash::small_hash(key, small_tabulation_table); });
   measure_hashfn("tabulation_large64",
                  [&](const HASH_64& key) { return TabulationHash::large_hash(key, large_tabulation_table); });

   measure_hashfn("city64", [](const HASH_64& key) { return CityHash::CityHash64(key); });
   measure_hashfn("city128_low", [](const HASH_64& key) { return Reduction::lower_half(CityHash::CityHash128(key)); });
   measure_hashfn("city128_upp", [](const HASH_64& key) { return Reduction::upper_half(CityHash::CityHash128(key)); });
   measure_hashfn("city128_xor", [](const HASH_64& key) { return Reduction::xor_both(CityHash::CityHash128(key)); });
   measure_hashfn("city128_city",
                  [](const HASH_64& key) { return Reduction::hash_128_to_64(CityHash::CityHash128(key)); });

   measure_hashfn("meow64_low", [](const HASH_64& key) { return MeowHash::hash64(key); });
   measure_hashfn("meow64_upp", [](const HASH_64& key) { return MeowHash::hash64<1>(key); });

   measure_hashfn("aqua_low", [](const HASH_64& key) { return AquaHash::hash64(key); });
   measure_hashfn("aqua_upp", [](const HASH_64& key) { return AquaHash::hash64<1>(key); });
}

void print_max_resource_usage(const Args& args) {
   auto spawned_thread_count = args.datasets.size() * args.load_factors.size();
   const auto max_thread_count = std::min(static_cast<size_t>(args.max_threads), spawned_thread_count);
   std::vector<size_t> thread_mem;
   size_t max_bytes = 0;
   for (const auto& dataset : args.datasets) {
      const auto path = std::filesystem::current_path() / dataset.filepath;
      const auto dataset_size = std::filesystem::file_size(path);
      max_bytes += dataset_size; // each dataset is loaded in memory once

      for (const auto& load_fac : args.load_factors) {
         thread_mem.emplace_back(static_cast<long double>(dataset_size) / load_fac);
      }
   }
   std::sort(thread_mem.rbegin(), thread_mem.rend());

   for (size_t i = 0; i < max_thread_count; i++) {
      max_bytes += thread_mem[i];
   }

   std::cout << "Will concurrently schedule <= " << max_thread_count
             << " threads while consuming <= " << max_bytes / (std::pow(1024, 3)) << " GB of ram" << std::endl;
}

int main(int argc, char* argv[]) {
   try {
      auto args = Args(argc, argv);
#ifdef VERBOSE
      print_max_resource_usage(args);
#endif

      CSV outfile(args.outfile, csv_columns);

      // Worker pool for speeding up the benchmarking
      std::mutex iomutex;
      std::counting_semaphore cpu_blocker(args.max_threads);
      std::vector<std::thread> threads{};

      // Precompute tabulation hash tables once (don't have to change per dataset)
      HASH_64 small_tabulation_table[0xFF];
      HASH_64 large_tabulation_table[sizeof(HASH_64)][0xFF];
      TabulationHash::gen_column(small_tabulation_table);
      TabulationHash::gen_table(large_tabulation_table);

      for (const auto& it : args.datasets) {
         // TODO: once we have more RAM we maybe should load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. i.e. move this load into threads after aquire()
         const auto dataset_ptr = std::make_shared<const std::vector<uint64_t>>(it.load(iomutex));

         for (auto load_factor : args.load_factors) {
            threads.emplace_back(std::thread([&, dataset_ptr, load_factor] {
               cpu_blocker.aquire();
               measure(it.name(), dataset_ptr, load_factor, outfile, iomutex, small_tabulation_table,
                       large_tabulation_table);
               cpu_blocker.release();
            }));
         }

#ifdef LOW_MEMORY
         for (auto& t : threads) {
            t.join();
         }
         threads.clear();
      }
#else
      }
      for (auto& t : threads) {
         t.join();
      }
      threads.clear();
#endif
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}