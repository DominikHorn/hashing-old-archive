#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>

#include <convenience.hpp>
#include <hashtable.hpp>
#include <learned_models.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/functors.hpp"
#include "include/learned_functors.hpp"

using Args = BenchmarkArgs::HashCollisionArgs;

const std::vector<std::string> csv_columns = {
   "dataset",
   "numelements",
   "load_factor",
   "sample_size",
   "bucket_size",
   "model",
   "reducer",
   "insert_nanoseconds_total",
   "insert_nanoseconds_per_key",
   "lookup_nanoseconds_total",
   "lookup_nanoseconds_per_key",
};

static void benchmark(const std::string& dataset_name, const std::shared_ptr<const std::vector<uint64_t>> dataset_ptr,
                      const double load_factor, CSV& outfile, std::mutex& iomutex) {
   const auto measure = [&](auto hashtable, const auto& sample_size) {
      const auto hash_name = hashtable.hash_name();
      const auto reducer_name = hashtable.reducer_name();

      const auto str = [](auto s) { return std::to_string(s); };
      std::map<std::string, std::string> datapoint({
         {"dataset", dataset_name},
         {"numelements", str(dataset_ptr->size())},
         {"load_factor", str(load_factor)},
         {"sample_size", str(sample_size)},
         {"bucket_size", str(hashtable.bucket_size())},
         {"model", hash_name},
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
      const auto stats = Benchmark::measure_hashtable(*dataset_ptr, hashtable);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") insert took "
                   << relative_to(stats.total_insert_ns, dataset_ptr->size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.total_insert_ns) << " s total), lookup took "
                   << relative_to(stats.total_lookup_ns, dataset_ptr->size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.total_lookup_ns) << " s total)" << std::endl;
      };
#endif

      datapoint.emplace("insert_nanoseconds_total", str(stats.total_insert_ns));
      datapoint.emplace("insert_nanoseconds_per_key", str(relative_to(stats.total_insert_ns, dataset_ptr->size())));
      datapoint.emplace("lookup_nanoseconds_total", str(stats.total_lookup_ns));
      datapoint.emplace("lookup_nanoseconds_per_key", str(relative_to(stats.total_lookup_ns, dataset_ptr->size())));

      // Write to csv
      outfile.write(datapoint);
   };

   /**
    * ====================================
    *        Learned Indices
    * ====================================
    */

   // Take a sample
   const double sample_size = 0.01; // Fix this for now at 0.01
   const auto sample_n = static_cast<size_t>(sample_size * static_cast<long double>(dataset_ptr->size()));
   const auto pgm_sample_fn = [&]() {
      std::vector<uint64_t> sample(sample_n, 0);
      if (sample_n == 0)
         return sample;
      if (sample_n == dataset_ptr->size()) {
         sample = *dataset_ptr;
         return sample;
      }

      // Random constant to ensure reproducibility for debugging. TODO: make truely random for benchmark/use varying constants (?) -> also adjust fisher yates shuffle and other such constants
      const uint64_t seed = 0x9E3779B9LU;
      std::default_random_engine gen(seed);
      std::uniform_int_distribution<uint64_t> dist(0, dataset_ptr->size() - 1);
      for (size_t i = 0; i < sample_n; i++) {
         const auto random_index = dist(gen);
         sample[i] = dataset_ptr->at(random_index);
      }

      return sample;
   };

   const auto sort_prepare = [](auto& sample) { std::sort(sample.begin(), sample.end()); };

   // Take a random sample
   auto sample = pgm_sample_fn();
   sort_prepare(sample);

   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset_ptr->size()) / static_cast<double>(load_factor));

   const auto cuckoo_4_num_buckets =
      Hashtable::Cuckoo<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                        FastrangeFunc<HASH_32>, FastrangeFunc<HASH_32>>::num_buckets(hashtable_size);
   const auto cuckoo_8_num_buckets =
      Hashtable::Cuckoo<uint64_t, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                        FastrangeFunc<HASH_32>, FastrangeFunc<HASH_32>>::num_buckets(hashtable_size);

   measure(Hashtable::Chained<uint64_t, uint32_t, 1, PGMHashFunc<HASH_64, 256, 0>, MinMaxCutoffFunc<HASH_64>>(
              hashtable_size, MinMaxCutoffFunc<HASH_64>(),
              PGMHashFunc<HASH_64, 256, 0>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, PGMHashFunc<HASH_64, 256, 0>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_4_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_4_num_buckets),
              PGMHashFunc<HASH_64, 256, 0>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, PGMHashFunc<HASH_64, 256, 0>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_8_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_8_num_buckets),
              PGMHashFunc<HASH_64, 256, 0>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);

   measure(Hashtable::Chained<uint64_t, uint32_t, 1, PGMHashFunc<HASH_64, 128, 4>, MinMaxCutoffFunc<HASH_64>>(
              hashtable_size, MinMaxCutoffFunc<HASH_64>(),
              PGMHashFunc<HASH_64, 128, 4>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, PGMHashFunc<HASH_64, 128, 4>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_4_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_4_num_buckets),
              PGMHashFunc<HASH_64, 128, 4>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, PGMHashFunc<HASH_64, 128, 4>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_8_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_8_num_buckets),
              PGMHashFunc<HASH_64, 128, 4>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);

   measure(Hashtable::Chained<uint64_t, uint32_t, 1, PGMHashFunc<HASH_64, 64, 1>, MinMaxCutoffFunc<HASH_64>>(
              hashtable_size, MinMaxCutoffFunc<HASH_64>(),
              PGMHashFunc<HASH_64, 64, 1>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, PGMHashFunc<HASH_64, 64, 1>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_4_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_4_num_buckets),
              PGMHashFunc<HASH_64, 64, 1>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, PGMHashFunc<HASH_64, 64, 1>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_8_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_8_num_buckets),
              PGMHashFunc<HASH_64, 64, 1>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);

   measure(Hashtable::Chained<uint64_t, uint32_t, 1, PGMHashFunc<HASH_64, 4, 4>, MinMaxCutoffFunc<HASH_64>>(
              hashtable_size, MinMaxCutoffFunc<HASH_64>(),
              PGMHashFunc<HASH_64, 4, 4>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, PGMHashFunc<HASH_64, 4, 4>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_4_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_4_num_buckets),
              PGMHashFunc<HASH_64, 4, 4>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, PGMHashFunc<HASH_64, 4, 4>, Murmur3FinalizerCuckoo2Func,
                             MinMaxCutoffFunc<HASH_64>, FastModuloFunc<HASH_64>>(
              cuckoo_8_num_buckets, MinMaxCutoffFunc<HASH_64>(), FastModuloFunc<HASH_64>(cuckoo_8_num_buckets),
              PGMHashFunc<HASH_64, 4, 4>(sample.begin(), sample.end(), hashtable_size)),
           sample_size);
}

int main(int argc, char* argv[]) {
   try {
      auto args = Args(argc, argv);
#ifdef VERBOSE
      std::vector<size_t> exec_mem;
      size_t max_bytes = 0;
      for (const auto& dataset : args.datasets) {
         const auto path = std::filesystem::current_path() / dataset.filepath;
         const auto dataset_size = std::filesystem::file_size(path);

         for (const auto& load_fac : args.load_factors) {
            const auto max_bucket_size =
               Hashtable::Chained<uint64_t, uint32_t, 4, Mult64Func, FastrangeFunc<HASH_64>>::bucket_byte_size();
            const auto base_ht_size =
               (static_cast<long double>(dataset_size - dataset.bytesPerValue) / (load_fac * dataset.bytesPerValue)) *
               max_bucket_size;
            const auto max_excess_buckets_size =
               (static_cast<long double>(dataset_size - 2 * dataset.bytesPerValue) / (dataset.bytesPerValue)) *
               max_bucket_size;
            //         const auto learned_index_size = (?)

            // Chained hashtable memory consumption upper estimate
            exec_mem.emplace_back(dataset_size + base_ht_size + max_excess_buckets_size);
         }
      }

      std::sort(exec_mem.rbegin(), exec_mem.rend());
      max_bytes += exec_mem[0];

      std::cout << "Will consume max <= " << max_bytes / (std::pow(1024, 3)) << " GB of ram" << std::endl;
#endif

      CSV outfile(args.outfile, csv_columns);

      // Worker pool for speeding up the benchmarking
      std::mutex iomutex;

      for (const auto& it : args.datasets) {
         // TODO: once we have more RAM we maybe should load the dataset per thread (prevent cache conflicts)
         //  and purely operate on thread local data. i.e. move this load into threads after aquire()
         const auto dataset_ptr = std::make_shared<const std::vector<uint64_t>>(it.load(iomutex));

         for (auto load_factor : args.load_factors) {
            benchmark(it.name(), dataset_ptr, load_factor, outfile, iomutex);
         }
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
