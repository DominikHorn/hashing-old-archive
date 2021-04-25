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

static void measure(const std::string& dataset_name, const std::shared_ptr<const std::vector<uint64_t>> dataset,
                    const double load_factor, const Args& args, CSV& outfile, std::mutex& iomutex) {
   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset->size()) / static_cast<double>(load_factor));

   // lambda to measure a given hash function with a given reducer. Will be entirely inlined by default
   std::vector<uint32_t> collision_counter(hashtable_size);
   const auto measure_model = [&](const std::string& model_name, const auto& samplefn, const auto& preparefn,
                                  const auto& buildfn, const auto& modelfn, const std::string& reducer_name,
                                  const auto& reducerfn) {
      // Take a random sample
      auto start_time = std::chrono::steady_clock::now();
      auto sample = samplefn();
      uint64_t sample_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      // Prepare the sample
      start_time = std::chrono::steady_clock::now();
      preparefn(sample);
      uint64_t prepare_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      // Build the model
      start_time = std::chrono::steady_clock::now();
      const auto model = buildfn(sample);
      uint64_t build_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      // Measure
      const auto stats = Benchmark::measure_collisions(
         *dataset, collision_counter, [&](const HASH_64& key) { return modelfn(model, hashtable_size, key); },
         reducerfn);

      // Sum up for easier access
      const auto total_ns = sample_ns + prepare_ns + build_ns + stats.inference_reduction_memaccess_total_ns;

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + model_name + ") took "
                   << relative_to(total_ns, dataset->size()) << " ns/key (" << nanoseconds_to_seconds(total_ns)
                   << " s total)" << std::endl;
      };
#endif

      const auto str = [](auto s) { return std::to_string(s); };
      outfile.write({
         {"dataset", dataset_name},
         {"numelements", str(dataset->size())},
         {"load_factor", str(load_factor)},
         {"model", model_name},
         {"reducer", reducer_name},
         {"sample_size", str(relative_to(sample.size(), dataset->size()))},
         {"min", str(stats.min)},
         {"max", str(stats.max)},
         {"std_dev", str(stats.std_dev)},
         {"empty_slots", str(stats.empty_slots)},
         {"empty_slots_percent", str(relative_to(stats.empty_slots, hashtable_size))},
         {"colliding_slots", str(stats.colliding_slots)},
         {"colliding_slots_percent", str(relative_to(stats.colliding_slots, hashtable_size))},
         {"total_colliding_keys", str(stats.total_colliding_keys)},
         {"total_colliding_keys_percent", str(relative_to(stats.total_colliding_keys, dataset->size()))},
         {"sample_nanoseconds_total", str(sample_ns)},
         {"sample_nanoseconds_per_key", str(relative_to(sample_ns, dataset->size()))},
         {"prepare_nanoseconds_total", str(prepare_ns)},
         {"prepare_nanoseconds_per_key", str(relative_to(prepare_ns, dataset->size()))},
         {"build_nanoseconds_total", str(build_ns)},
         {"build_nanoseconds_per_key", str(relative_to(build_ns, dataset->size()))},
         {"hashing_nanoseconds_total", str(stats.inference_reduction_memaccess_total_ns)},
         {"hashing_nanoseconds_per_key",
          str(relative_to(stats.inference_reduction_memaccess_total_ns, dataset->size()))},
         {"total_nanoseconds", str(total_ns)},
         {"total_nanoseconds_per_key", str(relative_to(total_ns, dataset->size()))},
      });
   };

   const auto sort_prepare = [](auto& sample) { std::sort(sample.begin(), sample.end()); };

   /** ============================
    *        Learned Models
    *  ============================
    */

   // TODO: test different reduction methods (optimize with unlikely() annotation etc)

   for (const auto sample_size : args.sample_sizes) {
      const auto sample_n = static_cast<size_t>(sample_size * static_cast<long double>(dataset->size()));
      const auto pgm_sample_fn = [&]() {
         std::vector<uint64_t> sample(sample_n, 0);
         if (sample_n == 0)
            return sample;
         if (sample_n == dataset->size()) {
            sample = *dataset;
            return sample;
         }

         // Random constant to ensure reproducibility for debugging. TODO: make truely random for benchmark/use varying constants (?) -> also adjust fisher yates shuffle and other such constants
         const uint64_t seed = 0x9E3779B9LU;
         std::default_random_engine gen(seed);
         std::uniform_int_distribution<uint64_t> dist(0, dataset->size() - 1);
         for (size_t i = 0; i < sample_n; i++) {
            const auto random_index = dist(gen);
            sample[i] = dataset->at(random_index);
         }

         return sample;
      };

      const auto pgm_hash = [&](const auto& pgm, const HASH_64& N, const HASH_64& key) {
         // Otherwise pgm will EXC_BAD_ACCESS
         if (unlikely(key == std::numeric_limits<HASH_64>::max())) {
            return N;
         }

         // Since we're training pgm on a sample, pos has to be scaled to fill the full [0, N]
         const auto sample_pos = static_cast<long double>(pgm.search(key).pos);
         const auto fac = static_cast<long double>(N) / static_cast<long double>(sample_n);
         return static_cast<HASH_64>(sample_pos * fac);
      };

      measure_model(
         "pgm_hash_eps64_epsrec1", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps16_epsrec1", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 16, 1>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps8_epsrec1", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 8, 1>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps64_epsrec0", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 64, 0>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps16_epsrec0", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 16, 0>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps8_epsrec0", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 8, 0>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);
   }
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

      for (const auto& it : args.datasets) {
         // TODO: once we are on a NUMA machine, we should maybe load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. i.e. move this load into threads after aquire()
         const auto dataset_ptr = std::make_shared<const std::vector<uint64_t>>(it.load(iomutex));

         for (const auto load_factor : args.load_factors) {
            threads.emplace_back(std::thread([&, dataset_ptr, load_factor] {
               cpu_blocker.aquire();
               measure(it.name(), dataset_ptr, load_factor, args, outfile, iomutex);
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
