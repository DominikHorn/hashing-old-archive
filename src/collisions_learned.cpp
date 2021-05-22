#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <convenience.hpp>
#include <learned_models.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/random_hash.hpp"

using Args = BenchmarkArgs::LearnedCollisionArgs;

const std::vector<std::string> csv_columns = {"dataset",
                                              "numelements",
                                              "load_factor",
                                              "model",
                                              "reducer",
                                              "sample_size",
                                              "min",
                                              "max",
                                              "std_dev",
                                              "empty_slots",
                                              "empty_slots_percent",
                                              "colliding_slots",
                                              "colliding_slots_percent",
                                              "total_colliding_keys",
                                              "total_colliding_keys_percent",
                                              "sample_nanoseconds_total",
                                              "sample_nanoseconds_per_key",
                                              "prepare_nanoseconds_total",
                                              "prepare_nanoseconds_per_key",
                                              "build_nanoseconds_total",
                                              "build_nanoseconds_per_key",
                                              "hashing_nanoseconds_total",
                                              "hashing_nanoseconds_per_key",
                                              "total_nanoseconds",
                                              "total_nanoseconds_per_key"};

template<class Hashfn, class Reducerfn, class Data>
static void measure(const std::string& dataset_name, const std::vector<Data>& dataset, const std::vector<Data>& sample,
                    std::vector<size_t>& collision_counter, const uint64_t& sample_ns, const uint64_t& prepare_ns,
                    CSV& outfile, std::mutex& iomutex) {
   const size_t N = dataset.size();
   const auto load_factor = static_cast<long double>(N) / static_cast<long double>(collision_counter.size());

   const auto str = [](auto s) { return std::to_string(s); };
   std::map<std::string, std::string> datapoint({
      {"dataset", dataset_name},
      {"numelements", str(N)},
      {"load_factor", str(load_factor)},
      {"model", Hashfn::name()},
      {"reducer", Reducerfn::name()},
      {"sample_size", str(relative_to(sample.size(), N))},
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

   // Build the model (e2e time)
   auto start_time = std::chrono::steady_clock::now();
   Hashfn hashfn(sample.begin(), sample.end(), N);
   uint64_t build_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

   // Measure
   const auto stats = Benchmark::measure_collisions<Hashfn, Reducerfn>(dataset, collision_counter, hashfn);

   // Sum up for easier access
   const auto total_ns = sample_ns + prepare_ns + build_ns + stats.inference_reduction_memaccess_total_ns;

#ifdef VERBOSE
   {
      std::unique_lock<std::mutex> lock(iomutex);
      std::cout << std::setw(55) << std::right << Reducerfn::name() + "(" + Hashfn::name() + ") took "
                << relative_to(total_ns, N) << " ns/key (" << nanoseconds_to_seconds(total_ns) << " s total)"
                << std::endl;
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
   datapoint.emplace("sample_nanoseconds_total", str(sample_ns));
   datapoint.emplace("sample_nanoseconds_per_key", str(relative_to(sample_ns, dataset.size())));
   datapoint.emplace("prepare_nanoseconds_total", str(prepare_ns));
   datapoint.emplace("prepare_nanoseconds_per_key", str(relative_to(prepare_ns, dataset.size())));
   datapoint.emplace("build_nanoseconds_total", str(build_ns));
   datapoint.emplace("build_nanoseconds_per_key", str(relative_to(build_ns, dataset.size())));
   datapoint.emplace("hashing_nanoseconds_total", str(stats.inference_reduction_memaccess_total_ns));
   datapoint.emplace("hashing_nanoseconds_per_key",
                     str(relative_to(stats.inference_reduction_memaccess_total_ns, dataset.size())));
   datapoint.emplace("total_nanoseconds", str(total_ns));
   datapoint.emplace("total_nanoseconds_per_key", str(relative_to(total_ns, dataset.size())));

   // Write to csv
   outfile.write(datapoint);
}

template<class Data>
static void measure(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                    const double sample_size, CSV& outfile, std::mutex& iomutex) {
   uint64_t sample_ns = 0, prepare_ns = 0;

   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset.size()) / static_cast<double>(load_factor));
   std::vector<size_t> collision_counter(hashtable_size);

   // Take a random sample
   const auto sample_n = static_cast<size_t>(sample_size * static_cast<long double>(dataset.size()));
   std::vector<uint64_t> sample(sample_n, 0);
   {
      auto start_time = std::chrono::steady_clock::now();
      if (sample_n == dataset.size()) {
         sample = dataset;
      } else if (sample_n > 0) {
         sample.resize(sample_n);

         // Random constant to ensure reproducibility for debugging.
         // TODO: make truely random for benchmark/use varying constants (?) -> also adjust other such constants (e.g. fisher yates shuffle)
         const uint64_t seed = 0x9E3779B9LU;
         std::default_random_engine gen(seed);
         std::uniform_int_distribution<uint64_t> dist(0, dataset.size() - 1);

         if (likely(sample_n > 2)) {
            Data& min = sample[0];
            Data& max = sample[sample_n - 1];

            min = std::numeric_limits<Data>::max();
            max = std::numeric_limits<Data>::min();
            for (const auto& key : dataset) {
               min = std::min(key, min);
               max = std::max(key, max);
            }
         }

         for (size_t i = 1; i < sample_n - 1; i++) {
            const auto random_index = dist(gen);
            sample[i] = dataset[random_index];
         }
      }
      sample_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());
   }

   // Prepare the sample
   {
      auto start_time = std::chrono::steady_clock::now();
      std::sort(sample.begin(), sample.end());
      prepare_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());
   }

   // TODO: enable rmi once implemented

   //   /// RMI
   //   measure<rmi::RMIHash<Data, 100>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns, prepare_ns,
   //                                                              outfile, iomutex);
   //   measure<rmi::RMIHash<Data, 1000>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns, prepare_ns,
   //                                                               outfile, iomutex);
   //   measure<rmi::RMIHash<Data, 10000>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns, prepare_ns,
   //                                                                outfile, iomutex);

   /// RadixSpline
   measure<rs::RadixSplineHash<Data>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter,
                                                                sample_ns, prepare_ns, outfile, iomutex);

   /// PGM (eps_rec 4)
   measure<PGMHash<Data, 256, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                            prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 128, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                            prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 64, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                           prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 16, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                           prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 4, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                          prepare_ns, outfile, iomutex);

   /// PGM (eps_rec 1)
   measure<PGMHash<Data, 256, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                            prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 128, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                            prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 64, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                           prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 16, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                           prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 4, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                          prepare_ns, outfile, iomutex);

   // PGM (eps_rec 0)
   measure<PGMHash<Data, 256, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                            prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 128, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                            prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 64, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                           prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 16, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                           prepare_ns, outfile, iomutex);
   measure<PGMHash<Data, 4, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, collision_counter, sample_ns,
                                                          prepare_ns, outfile, iomutex);
}

int main(int argc, char* argv[]) {
   try {
      auto args = Args(argc, argv);
#ifdef VERBOSE
      auto spawned_thread_count = args.datasets.size() * args.load_factors.size();
      const auto max_thread_count = std::min(static_cast<size_t>(args.max_threads), spawned_thread_count);
      std::vector<size_t> thread_mem;
      size_t max_bytes = 0;
      for (const auto& dataset : args.datasets) {
         const auto path = std::filesystem::current_path() / dataset.filepath;
         const auto dataset_size = std::filesystem::file_size(path);
         max_bytes += dataset_size; // each dataset is loaded in memory once

         for (const auto& load_fac : args.load_factors) {
            for (const auto& sample_size : args.sample_sizes) {
               size_t mem = 0;

               // collision counter
               mem += static_cast<double>(dataset_size) / load_fac;
               // sample
               mem += sample_size * dataset_size;
               // TODO: model
               //            thread_mem +=

               thread_mem.emplace_back(mem);
            }
         }
      }
      std::sort(thread_mem.rbegin(), thread_mem.rend());

      for (size_t i = 0; i < max_thread_count; i++) {
         max_bytes += thread_mem[i];
      }

      std::cout << "Will concurrently schedule <= " << max_thread_count
                << " threads while consuming <= " << max_bytes / (std::pow(1024, 3)) << " GB of ram" << std::endl;
#endif
      CSV outfile(args.outfile, csv_columns);

      // Worker pool for speeding up the benchmarking
      std::mutex iomutex;
      std_ext::counting_semaphore cpu_blocker(args.max_threads);
      std::vector<std::thread> threads{};

      for (const auto& it : args.datasets) {
         // TODO: once we are on a NUMA machine, we should maybe load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. i.e. move this load into threads after aquire()
         const auto dataset_ptr = std::make_shared<const std::vector<uint64_t>>(it.load(iomutex));

         for (const auto load_factor : args.load_factors) {
            for (const auto sample_size : args.sample_sizes) {
               threads.emplace_back(std::thread([&, dataset_ptr, load_factor] {
                  cpu_blocker.aquire();
                  measure(it.name(), *dataset_ptr, load_factor, sample_size, outfile, iomutex);
                  cpu_blocker.release();
               }));
            }
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
