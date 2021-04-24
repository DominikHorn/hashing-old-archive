#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <convenience.hpp>
#include <hashtable.hpp>
#include <learned_models.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"

using Args = BenchmarkArgs::HashCollisionArgs;

const std::vector<std::string> csv_columns = {
   "dataset",
   "numelements",
   "load_factor",
   "sample_size",
   "bucket_size",
   "model",
   "reducer",
   "sample_size",
   "insert_nanoseconds_total",
   "insert_nanoseconds_per_key",
   "lookup_nanoseconds_total",
   "lookup_nanoseconds_per_key",
};

template<typename T>
static void measure(const std::string& dataset_name, const std::shared_ptr<const std::vector<uint64_t>> dataset,
                    T& hashtable, const size_t bucket_size, const double load_factor, CSV& outfile,
                    std::mutex& iomutex) {
   const auto measure_model = [&](const std::string& model_name, const auto& samplefn, const auto& sample_size,
                                  const auto& preparefn, const auto& buildfn, const auto& modelfn,
                                  const std::string& reducer_name, const auto& reducerfn) {
      // Take a random sample
      auto sample = samplefn();
      preparefn(sample);
      const auto model = buildfn(sample);

      // Measure
      const auto stats = Benchmark::measure_hashtable(*dataset, hashtable, [&](const HASH_64& key, const size_t& N) {
         return reducerfn(modelfn(model, N, key), N);
      });

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + model_name + ") insert took "
                   << relative_to(stats.total_insert_ns, dataset->size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.total_insert_ns) << " s total), lookup took "
                   << relative_to(stats.total_lookup_ns, dataset->size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.total_lookup_ns) << " s total)" << std::endl;
      };
#endif

      const auto str = [](auto s) { return std::to_string(s); };
      outfile.write({
         {"dataset", dataset_name},
         {"numelements", str(dataset->size())},
         {"load_factor", str(load_factor)},
         {"sample_size", str(sample_size)},
         {"bucket_size", str(bucket_size)},
         {"model", model_name},
         {"reducer", reducer_name},
         {"insert_nanoseconds_total", str(stats.total_insert_ns)},
         {"insert_nanoseconds_per_key", str(relative_to(stats.total_insert_ns, dataset->size()))},
         {"lookup_nanoseconds_total", str(stats.total_lookup_ns)},
         {"lookup_nanoseconds_per_key", str(relative_to(stats.total_lookup_ns, dataset->size()))},
      });
   };

   /**
    * ====================================
    *        Learned Indices
    * ====================================
    */
   const double sample_size = 0.01; // Fix this for now at 0.01
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

   const auto sort_prepare = [](auto& sample) { std::sort(sample.begin(), sample.end()); };

   measure_model(
      "pgm_hash_eps64", pgm_sample_fn, sample_size, sort_prepare,
      [](const auto& sample) { return pgm::PGMHash<HASH_64, 64>(sample.begin(), sample.end()); }, //
      [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
      "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);

   measure_model(
      "pgm_hash_eps16", pgm_sample_fn, sample_size, sort_prepare,
      [](const auto& sample) { return pgm::PGMHash<HASH_64, 8>(sample.begin(), sample.end()); }, //
      [](const auto& pgm, const HASH_64& N, const HASH_64& key) { return pgm.hash(key, N); }, //
      "min_max_cutoff", Reduction::min_max_cutoff<HASH_64>);
}

void print_max_resource_usage(const Args& args) {
   std::vector<size_t> exec_mem;
   size_t max_bytes = 0;
   for (const auto& dataset : args.datasets) {
      const auto path = std::filesystem::current_path() / dataset.filepath;
      const auto dataset_size = std::filesystem::file_size(path);
      max_bytes += dataset_size; // each dataset is loaded in memory once

      for (const auto& load_fac : args.load_factors) {
         const auto base_ht_size =
            (static_cast<long double>(dataset_size - dataset.bytesPerValue) / (load_fac * dataset.bytesPerValue)) * 32;
         const auto max_excess_buckets_size =
            (static_cast<long double>(dataset_size - 2 * dataset.bytesPerValue) / (dataset.bytesPerValue)) * 32;
         //         const auto model_size = (?)

         // Chained hashtable memory consumption upper estimate
         exec_mem.emplace_back(base_ht_size + max_excess_buckets_size);
      }
   }

   std::sort(exec_mem.rbegin(), exec_mem.rend());
   max_bytes += exec_mem[0];

   std::cout << "Will consume max <= " << max_bytes / (std::pow(1024, 3)) << " GB of ram" << std::endl;
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

      for (const auto& it : args.datasets) {
         // TODO: once we have more RAM we maybe should load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. i.e. move this load into threads after aquire()
         const auto dataset_ptr = std::make_shared<const std::vector<uint64_t>>(it.load(iomutex));

         for (auto load_factor : args.load_factors) {
            // Theoretical slot count of a hashtable on which we want to measure collisions
            const auto hashtable_size =
               static_cast<uint64_t>(static_cast<double>(dataset_ptr->size()) / static_cast<double>(load_factor));

            // Bucket size 1
            {
               Hashtable::Chained<HASH_64, uint64_t, 1> chained_1(hashtable_size);
               measure(it.name(), dataset_ptr, chained_1, 1, load_factor, outfile, iomutex);
            }

            // Bucket size 2
            {
               Hashtable::Chained<HASH_64, uint64_t, 2> chained_2(hashtable_size);
               measure(it.name(), dataset_ptr, chained_2, 2, load_factor, outfile, iomutex);
            }

            // Bucket size 4
            {
               Hashtable::Chained<HASH_64, uint64_t, 4> chained_4(hashtable_size);
               measure(it.name(), dataset_ptr, chained_4, 4, load_factor, outfile, iomutex);
            }
         }
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
