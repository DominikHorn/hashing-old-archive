#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>

#include <convenience.hpp>
#include <hashtable.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/functors.hpp"

using Args = BenchmarkArgs::HashCollisionArgs;

const std::vector<std::string> csv_columns = {
   "dataset",
   "numelements",
   "load_factor",
   "bucket_size",
   "hashtable",
   "hash",
   "reducer",
   "insert_nanoseconds_total",
   "insert_nanoseconds_per_key",
   "lookup_nanoseconds_total",
   "lookup_nanoseconds_per_key",
};

static void benchmark(const std::string& dataset_name, const std::shared_ptr<const std::vector<uint64_t>> dataset,
                      const double load_factor, CSV& outfile, std::mutex& iomutex) {
   // lambda to measure a given hash function with a given reducer. Will be entirely inlined by default
   const auto measure = [&](auto hashtable) {
      const auto hash_name = hashtable.hash_name();
      const auto reducer_name = hashtable.reducer_name();

      const auto str = [](auto s) { return std::to_string(s); };
      std::map<std::string, std::string> datapoint({
         {"dataset", dataset_name},
         {"numelements", str(dataset->size())},
         {"load_factor", str(load_factor)},
         {"bucket_size", str(hashtable.bucket_size())},
         {"hashtable", hashtable.name()},
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

      try {
         // Measure
         const auto stats = Benchmark::measure_hashtable(*dataset, hashtable);

#ifdef VERBOSE
         {
            std::unique_lock<std::mutex> lock(iomutex);
            std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") insert took "
                      << relative_to(stats.total_insert_ns, dataset->size()) << " ns/key ("
                      << nanoseconds_to_seconds(stats.total_insert_ns) << " s total), lookup took "
                      << relative_to(stats.total_lookup_ns, dataset->size()) << " ns/key ("
                      << nanoseconds_to_seconds(stats.total_lookup_ns) << " s total)" << std::endl;
         };
#endif

         datapoint.emplace("insert_nanoseconds_total", str(stats.total_insert_ns));
         datapoint.emplace("insert_nanoseconds_per_key", str(relative_to(stats.total_insert_ns, dataset->size())));
         datapoint.emplace("lookup_nanoseconds_total", str(stats.total_lookup_ns));
         datapoint.emplace("lookup_nanoseconds_per_key", str(relative_to(stats.total_lookup_ns, dataset->size())));

         // Write to csv
         outfile.write(datapoint);
      } catch (const std::exception& e) {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") failed: " << e.what()
                   << std::endl;
      }
   };

   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset->size()) / static_cast<double>(load_factor));

   /// Chained mult
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, Mult64Func, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, Mult64Func, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, Mult64Func, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, Mult64Func, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, Mult64Func, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, Mult64Func, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, Mult64Func, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, Mult64Func, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, Mult64Func, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));

   /// Chained mult-add
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, MultAdd64Func, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, MultAdd64Func, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, MultAdd64Func, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, MultAdd64Func, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, MultAdd64Func, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, MultAdd64Func, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, MultAdd64Func, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, MultAdd64Func, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, MultAdd64Func, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));

   /// Chained murmur3
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));

   /// Chained aquahash
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, AquaLowFunc, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, AquaLowFunc, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 1, AquaLowFunc, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, AquaLowFunc, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, AquaLowFunc, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 2, AquaLowFunc, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, AquaLowFunc, FastrangeFunc<HASH_32>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, AquaLowFunc, FastrangeFunc<HASH_64>>(hashtable_size));
   measure(Hashtable::Chained<uint64_t, uint32_t, 4, AquaLowFunc, FastModuloFunc<HASH_64>>(
      hashtable_size, FastModuloFunc<HASH_64>(hashtable_size)));

   /*
    * ===============
    *     Cuckoo
    * ===============
    */

   const auto cuckoo_4_num_buckets =
      Hashtable::Cuckoo<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                        FastrangeFunc<HASH_32>, FastrangeFunc<HASH_32>>::num_buckets(hashtable_size);
   const auto cuckoo_8_num_buckets =
      Hashtable::Cuckoo<uint64_t, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                        FastrangeFunc<HASH_32>, FastrangeFunc<HASH_32>>::num_buckets(hashtable_size);

   /// Cuckoo murmur + murmur(xor) -> Stanford implementation
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastrangeFunc<HASH_32>, FastrangeFunc<HASH_32>>(cuckoo_4_num_buckets));
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastrangeFunc<HASH_64>, FastrangeFunc<HASH_64>>(cuckoo_4_num_buckets));
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 4, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastModuloFunc<HASH_64>, FastModuloFunc<HASH_64>>(
      cuckoo_4_num_buckets, FastModuloFunc<HASH_64>(cuckoo_4_num_buckets),
      FastModuloFunc<HASH_64>(cuckoo_4_num_buckets)));
   measure(Hashtable::Cuckoo<uint32_t, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastrangeFunc<HASH_32>, FastrangeFunc<HASH_32>>(cuckoo_8_num_buckets));
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastrangeFunc<HASH_64>, FastrangeFunc<HASH_64>>(cuckoo_8_num_buckets));
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastrangeFunc<HASH_64>, FastrangeFunc<HASH_64>>(cuckoo_8_num_buckets));
   measure(Hashtable::Cuckoo<uint64_t, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func,
                             FastModuloFunc<HASH_64>, FastModuloFunc<HASH_64>>(
      cuckoo_8_num_buckets, FastModuloFunc<HASH_64>(cuckoo_8_num_buckets),
      FastModuloFunc<HASH_64>(cuckoo_8_num_buckets)));

   //   // More significant bits supposedly are of higher quality for multiplicative methods -> compute
   //   // how much we need to shift/rotate to throw away the least/make 'high quality bits' as prominent as possible
   //   const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);
   //   measure_hashfn("multadd64_shift" + std::to_string(p),
   //                  [p](const HASH_64& key) { return MultAddHash::multadd64_hash(key, p); });
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

      std::mutex iomutex;
      CSV outfile(args.outfile, csv_columns);

      for (const auto& it : args.datasets) {
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
