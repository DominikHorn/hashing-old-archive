#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>

#include <convenience.hpp>
#include <hashing.hpp>
#include <hashtable.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"

using Args = BenchmarkArgs::HashCollisionArgs;

const std::vector<std::string> csv_columns = {
   "dataset",
   "numelements",
   "load_factor",
   "bucket_size",
   "hash",
   "reducer",
   "insert_nanoseconds_total",
   "insert_nanoseconds_per_key",
   "lookup_nanoseconds_total",
   "lookup_nanoseconds_per_key",
};

template<typename T>
static void measure(const std::string& dataset_name, const std::shared_ptr<const std::vector<uint64_t>> dataset,
                    T& hashtable, const size_t bucket_size, const double load_factor, CSV& outfile, std::mutex& iomutex,
                    const HASH_64 (&small_tabulation_table)[0xFF],
                    const HASH_64 (&large_tabulation_table)[sizeof(HASH_64)][0xFF]) {
   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto hashtable_size =
      static_cast<uint64_t>(static_cast<double>(dataset->size()) / static_cast<double>(load_factor));

   // lambda to measure a given hash function with a given reducer. Will be entirely inlined by default
   const auto measure_hashfn_with_reducer = [&](const std::string& hash_name, const std::string& reducer_name,
                                                const auto& hashfn) {
      const auto str = [](auto s) { return std::to_string(s); };
      std::map<std::string, std::string> datapoint({
         {"dataset", dataset_name},
         {"numelements", str(dataset->size())},
         {"load_factor", str(load_factor)},
         {"bucket_size", str(bucket_size)},
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
      const auto stats = Benchmark::measure_hashtable(*dataset, hashtable, hashfn);

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
   };

   // Build load factor specific auxiliary data
   const auto magic_div = Reduction::make_magic_divider(static_cast<HASH_64>(hashtable_size));

   // measures a hash function using every reducer
   const auto measure_hashfn = [&](const std::string& hash_name, const auto& hashfn) {
      const auto hash_fastrange32 = [&](const HASH_64& key, const size_t& N) {
         return Reduction::fastrange<HASH_32>(hashfn(key), N);
      };
      const auto hash_fastrange64 = [&](const HASH_64& key, const size_t& N) {
         return Reduction::fastrange<HASH_64>(hashfn(key), N);
      };
      const auto hash_fast_modulo = [&](const HASH_64& key, const size_t& N) {
         return Reduction::magic_modulo(hashfn(key), N, magic_div);
      };

      measure_hashfn_with_reducer(hash_name, "fastrange32", hash_fastrange32);
      measure_hashfn_with_reducer(hash_name, "fastrange64", hash_fastrange64);
      measure_hashfn_with_reducer(hash_name, "fast_modulo", hash_fast_modulo);
   };

   measure_hashfn("mult64", [](const HASH_64& key) { return MultHash::mult64_hash(key); });
   measure_hashfn("multadd64", [](const HASH_64& key) { return MultAddHash::multadd64_hash(key); });

   // More significant bits supposedly are of higher quality for multiplicative methods -> compute
   // how much we need to shift/rotate to throw away the least/make 'high quality bits' as prominent as possible
   const auto p = (sizeof(hashtable_size) * 8) - __builtin_clz(hashtable_size - 1);
   measure_hashfn("multadd64_shift" + std::to_string(p),
                  [p](const HASH_64& key) { return MultAddHash::multadd64_hash(key, p); });

   measure_hashfn("murmur3_fin64", [](const HASH_64& key) { return MurmurHash3::finalize_64(key); });
   //   measure_hashfn("meow64_low", [](const HASH_64& key) { return MeowHash::hash64(key); });
   measure_hashfn("aqua_low", [](const HASH_64& key) { return AquaHash::hash64(key); });
}

void print_max_resource_usage(const Args& args) {
   std::vector<size_t> exec_mem;
   size_t max_bytes = 0;
   for (const auto& dataset : args.datasets) {
      const auto path = std::filesystem::current_path() / dataset.filepath;
      const auto dataset_size = std::filesystem::file_size(path);

      for (const auto& load_fac : args.load_factors) {
         const auto max_bucket_size = Hashtable::Chained<uint64_t, uint32_t, 4>::bucket_size();
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
}

int main(int argc, char* argv[]) {
   try {
      auto args = Args(argc, argv);
#ifdef VERBOSE
      print_max_resource_usage(args);
#endif

      std::mutex iomutex;
      CSV outfile(args.outfile, csv_columns);

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
            // Theoretical slot count of a hashtable on which we want to measure collisions
            const auto hashtable_size =
               static_cast<uint64_t>(static_cast<double>(dataset_ptr->size()) / static_cast<double>(load_factor));

            // Bucket size 1
            {
               Hashtable::Chained<HASH_64, uint32_t, 1> chained_1(hashtable_size);
               measure(it.name(), dataset_ptr, chained_1, 1, load_factor, outfile, iomutex, small_tabulation_table,
                       large_tabulation_table);
            }

            // Bucket size 2
            {
               Hashtable::Chained<HASH_64, uint32_t, 2> chained_2(hashtable_size);
               measure(it.name(), dataset_ptr, chained_2, 2, load_factor, outfile, iomutex, small_tabulation_table,
                       large_tabulation_table);
            }

            // Bucket size 4
            {
               Hashtable::Chained<HASH_64, uint32_t, 4> chained_4(hashtable_size);
               measure(it.name(), dataset_ptr, chained_4, 4, load_factor, outfile, iomutex, small_tabulation_table,
                       large_tabulation_table);
            }
         }
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
