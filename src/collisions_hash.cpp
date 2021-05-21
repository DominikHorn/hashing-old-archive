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

template<class Hashfn, class Reducerfn, class Data>
static void measure(const std::string& dataset_name, const std::vector<Data>& dataset,
                    std::vector<size_t>& collision_counter, CSV& outfile, std::mutex& iomutex,
                    Hashfn hashfn = Hashfn()) {
   const auto load_factor =
      static_cast<long double>(dataset.size()) / static_cast<long double>(collision_counter.size());

   const auto str = [](auto s) { return std::to_string(s); };
   std::map<std::string, std::string> datapoint({
      {"dataset", dataset_name},
      {"numelements", str(dataset.size())},
      {"load_factor", str(load_factor)},
      {"hash", Hashfn::name()},
      {"reducer", Reducerfn::name()},
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
   const auto stats = Benchmark::measure_collisions<Hashfn, Reducerfn>(dataset, collision_counter, hashfn);

#ifdef VERBOSE
   {
      std::unique_lock<std::mutex> lock(iomutex);
      std::cout << std::setw(55) << std::right << Reducerfn::name() + "(" + Hashfn::name() + ") took "
                << relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size()) << " ns/key ("
                << nanoseconds_to_seconds(stats.inference_reduction_memaccess_total_ns) << " s total)" << std::endl;
   };
#endif

   datapoint.emplace("min", str(stats.min));
   datapoint.emplace("max", str(stats.max));
   datapoint.emplace("std_dev", str(stats.std_dev));
   datapoint.emplace("empty_slots", str(stats.empty_slots));
   datapoint.emplace("empty_slots_percent", str(relative_to(stats.empty_slots, collision_counter.size())));
   datapoint.emplace("colliding_slots", str(stats.colliding_slots));
   datapoint.emplace("colliding_slots_percent", str(relative_to(stats.colliding_slots, collision_counter.size())));
   datapoint.emplace("total_colliding_keys", str(stats.total_colliding_keys));
   datapoint.emplace("total_colliding_keys_percent", str(relative_to(stats.total_colliding_keys, dataset.size())));
   datapoint.emplace("nanoseconds_total", str(stats.inference_reduction_memaccess_total_ns));
   datapoint.emplace("nanoseconds_per_key",
                     str(relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size())));

   // Write to csv
   outfile.write(datapoint);
};

template<class Hashfn, class Data>
static void measure64(const std::string& dataset_name, const std::vector<Data>& dataset,
                      std::vector<size_t>& collision_counter, CSV& outfile, std::mutex& iomutex,
                      Hashfn hashfn = Hashfn()) {
   using namespace Reduction;
   //   measure<Hashfn, DoNothing<HASH_64>>(dataset_name, dataset, collision_counter, outfile, iomutex, hashfn);
   measure<Hashfn, Fastrange<HASH_32>>(dataset_name, dataset, collision_counter, outfile, iomutex, hashfn);
   measure<Hashfn, Fastrange<HASH_64>>(dataset_name, dataset, collision_counter, outfile, iomutex, hashfn);
   //   measure<Hashfn, Modulo<HASH_64>>(dataset_name, dataset, collision_counter, outfile, iomutex);
   measure<Hashfn, FastModulo<HASH_64>>(dataset_name, dataset, collision_counter, outfile, iomutex, hashfn);
   //   measure<Hashfn, BranchlessFastModulo<HASH_64>>(dataset_name, collision_counter, dataset, outfile, iomutex);
};

template<class Hashfn, class Dataset>
static void measure128(const std::string& dataset_name, const Dataset& dataset, std::vector<size_t>& collision_counter,
                       CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;
   measure64<Reduction::Lower<Hashfn>>(dataset_name, dataset, collision_counter, outfile, iomutex);
   measure64<Reduction::Higher<Hashfn>>(dataset_name, dataset, collision_counter, outfile, iomutex);
   measure64<Reduction::Xor<Hashfn>>(dataset_name, dataset, collision_counter, outfile, iomutex);
   measure64<Reduction::City<Hashfn>>(dataset_name, dataset, collision_counter, outfile, iomutex);
}

template<class Data>
static void benchmark(const std::string& dataset_name, const std::shared_ptr<const std::vector<Data>> dataset,
                      const double load_factor, CSV& outfile, std::mutex& iomutex) {
   // Theoretical address count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<size_t>(static_cast<double>(dataset->size()) / static_cast<double>(load_factor));
   std::vector<size_t> collision_counter(hashtable_size);

   /// Baseline
   {
      RandomHash<Data> rand_uni(hashtable_size);
      measure64<RandomHash<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex, rand_uni);
   }

   measure64<PrimeMultiplicationHash64>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<FibonacciHash64>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<FibonacciPrimeHash64>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<MultAddHash64>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   // More significant bits supposedly are of higher quality for multiplicative methods -> compute
   // how much we need to shift/rotate to throw away the least/make 'high quality bits' as prominent as possible
   constexpr auto p = (sizeof(Data) * 8) - __builtin_clzll(200000000 - 1);
   measure64<PrimeMultiplicationShiftHash64<p>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<FibonacciShiftHash64<p>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<FibonacciPrimeShiftHash64<p>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<MultAddShiftHash64<p>>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   // TODO: measure rotation instead of shift?
   //            const unsigned int rot = 64 - p;
   //            measure_hashfn("mult64_rotate" + std::to_string(rot),
   //                           [&](HASH_64 key) { return rotr(MultHash::mult64_hash(key), rot); });

   measure128<Murmur3Hash128<>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<MurmurFinalizer<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   measure64<XXHash64<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<XXHash3<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure128<XXHash3_128<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   // SmallTabulation is bad since higher bits are always 0 & table[0] ^ table[0] = 0, i.e., we yeet many bytes entirely ;/
   measure64<SmallTabulationHash<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<MediumTabulationHash<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<LargeTabulationHash<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   measure64<CityHash64<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure128<CityHash128<Data>>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   measure64<MeowHash64<Data, 0>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<MeowHash64<Data, 1>>(dataset_name, *dataset, collision_counter, outfile, iomutex);

   measure64<AquaHash<Data, 0>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
   measure64<AquaHash<Data, 1>>(dataset_name, *dataset, collision_counter, outfile, iomutex);
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
      std_ext::counting_semaphore cpu_blocker(args.max_threads);
      std::vector<std::thread> threads{};

      for (const auto& it : args.datasets) {
         // TODO: once we have more RAM we maybe should load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. i.e. move this load into threads after aquire()
         const auto dataset_ptr = std::make_shared<const std::vector<uint64_t>>(it.load(iomutex));

         for (auto load_factor : args.load_factors) {
            threads.emplace_back(std::thread([&, dataset_ptr, load_factor] {
               cpu_blocker.aquire();
               benchmark(it.name(), dataset_ptr, load_factor, outfile, iomutex);
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