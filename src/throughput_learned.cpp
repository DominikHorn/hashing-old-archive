#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <convenience.hpp>
#include <learned_models.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"

using Args = BenchmarkArgs::LearnedThroughputArgs;

const std::vector<std::string> csv_columns = {"dataset",
                                              "numelements",
                                              "model",
                                              "reducer",
                                              "sample_size",
                                              "sample_nanoseconds_total",
                                              "sample_nanoseconds_per_key",
                                              "prepare_nanoseconds_total",
                                              "prepare_nanoseconds_per_key",
                                              "build_nanoseconds_total",
                                              "build_nanoseconds_per_key",
                                              "hashing_nanoseconds_total",
                                              "hashing_nanoseconds_per_key",
                                              "total_nanoseconds",
                                              "total_nanoseconds_per_key",
                                              "benchmark_repeat_cnt"};

static void measure(const std::string& dataset_name, const std::vector<uint64_t>& dataset, const Args& args,
                    CSV& outfile, std::mutex& iomutex) {
   const auto hashtable_size = dataset.size();

   // lambda to measure a given hash function with a given reducer. Will be entirely inlined by default
   const auto measure_model = [&](const std::string& model_name, const auto& samplefn, const auto& preparefn,
                                  const auto& buildfn, const auto& modelfn, const std::string& reducer_name,
                                  const auto& reducerfn) {
      // Take a random sample
      auto start_time = std::chrono::steady_clock::now();
      auto sample = samplefn();
      uint64_t sample_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

      const auto str = [](auto s) { return std::to_string(s); };
      std::map<std::string, std::string> datapoint({{"dataset", dataset_name},
                                                    {"numelements", str(dataset.size())},
                                                    {"model", model_name},
                                                    {"reducer", reducer_name},
                                                    {"sample_size", str(relative_to(sample.size(), dataset.size()))}});

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
      const auto stats = Benchmark::measure_throughput(
         dataset, [&](const HASH_64& key) { return modelfn(model, hashtable_size, key); }, reducerfn);

      // Sum up for easier access
      const auto total_ns = sample_ns + prepare_ns + build_ns + stats.average_total_inference_reduction_ns;

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + model_name + ") took "
                   << relative_to(total_ns, dataset.size()) << " ns/key (" << nanoseconds_to_seconds(total_ns)
                   << " s total)" << std::endl;
      };
#endif

      datapoint.emplace("sample_nanoseconds_total", str(sample_ns));
      datapoint.emplace("sample_nanoseconds_per_key", str(relative_to(sample_ns, dataset.size())));
      datapoint.emplace("prepare_nanoseconds_total", str(prepare_ns));
      datapoint.emplace("prepare_nanoseconds_per_key", str(relative_to(prepare_ns, dataset.size())));
      datapoint.emplace("build_nanoseconds_total", str(build_ns));
      datapoint.emplace("build_nanoseconds_per_key", str(relative_to(build_ns, dataset.size())));
      datapoint.emplace("hashing_nanoseconds_total", str(stats.average_total_inference_reduction_ns));
      datapoint.emplace("hashing_nanoseconds_per_key",
                        str(relative_to(stats.average_total_inference_reduction_ns, dataset.size())));
      datapoint.emplace("total_nanoseconds", str(total_ns));
      datapoint.emplace("total_nanoseconds_per_key", str(relative_to(total_ns, dataset.size())));
      datapoint.emplace("benchmark_repeat_cnt", str(stats.repeatCnt));

      // Write to csv
      outfile.write(datapoint);
   };

   const auto sort_prepare = [](auto& sample) { std::sort(sample.begin(), sample.end()); };

   /** ============================
    *        Learned Models
    *  ============================
    */

   // TODO: test different reduction methods (optimize with unlikely() annotation etc)

   for (const auto sample_size : args.sample_sizes) {
      const auto sample_n = static_cast<size_t>(sample_size * static_cast<long double>(dataset.size()));
      const auto pgm_sample_fn = [&]() {
         std::vector<uint64_t> sample(sample_n, 0);
         if (sample_n == 0)
            return sample;
         if (sample_n == dataset.size()) {
            sample = dataset;
            return sample;
         }

         // Random constant to ensure reproducibility for debugging. TODO: make truely random for benchmark/use varying constants (?) -> also adjust fisher yates shuffle and other such constants
         const uint64_t seed = 0x9E3779B9LU;
         std::default_random_engine gen(seed);
         std::uniform_int_distribution<uint64_t> dist(0, dataset.size() - 1);
         for (size_t i = 0; i < sample_n; i++) {
            const auto random_index = dist(gen);
            sample[i] = dataset[random_index];
         }

         return sample;
      };

      measure_model(
         "pgm_hash_eps256_epsrec4", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 256, 4>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps64_epsrec4", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 64, 4>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps16_epsrec4", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 16, 4>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps4_epsrec4", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps256_epsrec1", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 256, 1>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

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
         "pgm_hash_eps4_epsrec1", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 4, 1>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

      measure_model(
         "pgm_hash_eps256_epsrec0", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end()); }, //
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
         "pgm_hash_eps4_epsrec0", pgm_sample_fn, sort_prepare,
         [](const auto& sample) { return pgm::PGMHash<HASH_64, 4, 0>(sample.begin(), sample.end()); }, //
         [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
         "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);
   }
}

void print_max_resource_usage(const Args& args) {
   auto spawned_thread_count = args.datasets.size();
   const auto max_thread_count = std::min(static_cast<size_t>(args.max_threads), spawned_thread_count);
   std::vector<size_t> thread_mem;
   size_t max_bytes = 0;
   for (const auto& dataset : args.datasets) {
      const auto path = std::filesystem::current_path() / dataset.filepath;
      const auto dataset_size = std::filesystem::file_size(path);
      max_bytes += dataset_size; // each dataset is loaded in memory once

      for (const auto& sample_size : args.sample_sizes) {
         size_t mem = 0;

         // sample
         mem += sample_size * dataset_size;
         // TODO: model
         //            thread_mem +=

         thread_mem.emplace_back(mem);
      }
   }
   std::sort(thread_mem.rbegin(), thread_mem.rend());

   for (size_t i = 0; i < max_thread_count; i++) {
      max_bytes += thread_mem[i];
   }

   std::cout << "Will concurrently schedule <= " << max_thread_count
             << " threads while consuming <= " << static_cast<long double>(max_bytes / (std::pow(1024, 3)))
             << " GB of ram" << std::endl;
}

int main(int argc, char* argv[]) {
   try {
      Args args(argc, argv);
#if VERBOSE
      print_max_resource_usage(args);
#endif

      CSV outfile(args.outfile, csv_columns);

      // Worker pool for speeding up the benchmarking
      std::mutex iomutex;
      std::counting_semaphore cpu_blocker(args.max_threads);
      std::vector<std::thread> threads{};

      for (const auto& it : args.datasets) {
         threads.emplace_back(std::thread([&, it] {
            cpu_blocker.aquire();

            auto dataset = it.load(iomutex);
            measure(it.name(), dataset, args, outfile, iomutex);

            cpu_blocker.release();
         }));
      }

      for (auto& t : threads) {
         t.join();
      }
      threads.clear();
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}