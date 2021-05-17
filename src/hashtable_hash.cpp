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
#include "include/functors/hash_functors.hpp"
#include "include/functors/reduction_functors.hpp"

using Args = BenchmarkArgs::HashCollisionArgs;

const std::vector<std::string> csv_columns = {
   // General statistics
   "dataset", "numelements", "load_factor", "bucket_size", "hashtable", "hash", "reducer", "insert_nanoseconds_total",
   "insert_nanoseconds_per_key", "lookup_nanoseconds_total", "lookup_nanoseconds_per_key",

   // Cuckoo custom statistics
   "primary_key_ratio",

   // Chained custom statistics
   "empty_buckets", "min_chain_length", "max_chain_length", "total_chain_pointer_count",

   // Probing custom statistics
   "min_psl", "max_psl", "total_psl"

   //
};

template<class T>
static void benchmark(const std::string& dataset_name, const std::shared_ptr<const std::vector<T>> dataset,
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

      // Allocate hashtable memory
      hashtable.allocate();

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

         // Make sure we collect more insight based on hashtable
         for (const auto& stat : hashtable.lookup_statistics(*dataset)) {
            datapoint.emplace(stat);
         }

         // Write to csv
         outfile.write(datapoint);
      } catch (const std::exception& e) {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") failed: " << e.what()
                   << std::endl;
      }
   };

   // Theoretical slot count of a hashtable on which we want to measure collisions
   const auto ht_capacity =
      static_cast<uint64_t>(static_cast<double>(dataset->size()) / static_cast<double>(load_factor));

   /**
    * ===============
    *    Chained
    * ===============
    */

   /// mult
   measure(Hashtable::Chained<T, uint32_t, 1, PrimeMultiplicationHash64, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, PrimeMultiplicationHash64, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, PrimeMultiplicationHash64, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, PrimeMultiplicationHash64, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, PrimeMultiplicationHash64, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, PrimeMultiplicationHash64, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, PrimeMultiplicationHash64, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, PrimeMultiplicationHash64, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, PrimeMultiplicationHash64, FastModuloFunc<HASH_64>>(ht_capacity));

   /// mult-add
   measure(Hashtable::Chained<T, uint32_t, 1, MultAdd64Func, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, MultAdd64Func, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, MultAdd64Func, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, MultAdd64Func, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, MultAdd64Func, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, MultAdd64Func, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, MultAdd64Func, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, MultAdd64Func, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, MultAdd64Func, FastModuloFunc<HASH_64>>(ht_capacity));

   /// murmur3 finalizer
   measure(Hashtable::Chained<T, uint32_t, 1, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>>(ht_capacity));

   /// aquahash
   measure(Hashtable::Chained<T, uint32_t, 1, AquaLowFunc, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, AquaLowFunc, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 1, AquaLowFunc, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, AquaLowFunc, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, AquaLowFunc, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 2, AquaLowFunc, FastModuloFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, AquaLowFunc, FastrangeFunc<HASH_32>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, AquaLowFunc, FastrangeFunc<HASH_64>>(ht_capacity));
   measure(Hashtable::Chained<T, uint32_t, 4, AquaLowFunc, FastModuloFunc<HASH_64>>(ht_capacity));

   /**
    * ===============
    *    Probing
    * ===============
    */

   /// Linear Murmur finalizer
   measure(Hashtable::Probing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>, Hashtable::LinearProbingFunc>(
      ht_capacity));
   measure(Hashtable::Probing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>, Hashtable::LinearProbingFunc>(
      ht_capacity));
   measure(Hashtable::Probing<T, uint32_t, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>, Hashtable::LinearProbingFunc>(
      ht_capacity));

   /// Quadratic Murmur finalizer
   measure(
      Hashtable::Probing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>, Hashtable::QuadraticProbingFunc>(
         ht_capacity));
   measure(
      Hashtable::Probing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>, Hashtable::QuadraticProbingFunc>(
         ht_capacity));
   measure(
      Hashtable::Probing<T, uint32_t, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>, Hashtable::QuadraticProbingFunc>(
         ht_capacity));

   /**
    * ==============
    *   Robin Hood
    * ==============
    */
   /// Linear Murmur finalizer
   measure(Hashtable::RobinhoodProbing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>,
                                       Hashtable::LinearProbingFunc>(ht_capacity));
   measure(Hashtable::RobinhoodProbing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>,
                                       Hashtable::LinearProbingFunc>(ht_capacity));
   measure(Hashtable::RobinhoodProbing<T, uint32_t, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>,
                                       Hashtable::LinearProbingFunc>(ht_capacity));

   /// Quadratic Murmur finalizer
   measure(Hashtable::RobinhoodProbing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_32>,
                                       Hashtable::QuadraticProbingFunc>(ht_capacity));
   measure(Hashtable::RobinhoodProbing<T, uint32_t, Murmur3FinalizerFunc, FastrangeFunc<HASH_64>,
                                       Hashtable::QuadraticProbingFunc>(ht_capacity));
   measure(Hashtable::RobinhoodProbing<T, uint32_t, Murmur3FinalizerFunc, FastModuloFunc<HASH_64>,
                                       Hashtable::QuadraticProbingFunc>(ht_capacity));

   /**
    * ===============
    *     Cuckoo
    * ===============
    */

   /// Balanced Cuckoo murmur + murmur(xor) -> Stanford implementation
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastrangeFunc<HASH_32>,
                             FastrangeFunc<HASH_32>, Hashtable::BalancedKicking>(ht_capacity));
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastrangeFunc<HASH_64>,
                             FastrangeFunc<HASH_64>, Hashtable::BalancedKicking>(ht_capacity));
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastModuloFunc<HASH_64>,
                             FastModuloFunc<HASH_64>, Hashtable::BalancedKicking>(ht_capacity));

   /// Unbiased Cuckoo murmur + murmur(xor) -> Stanford implementation
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastrangeFunc<HASH_32>,
                             FastrangeFunc<HASH_32>, Hashtable::UnbiasedKicking>(ht_capacity));
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastrangeFunc<HASH_64>,
                             FastrangeFunc<HASH_64>, Hashtable::UnbiasedKicking>(ht_capacity));
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastModuloFunc<HASH_64>,
                             FastModuloFunc<HASH_64>, Hashtable::UnbiasedKicking>(ht_capacity));

   /// Biases_10% Cuckoo murmur + murmur(xor) -> Stanford implementation
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastrangeFunc<HASH_32>,
                             FastrangeFunc<HASH_32>, Hashtable::BiasedKicking<26>>(ht_capacity));
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastrangeFunc<HASH_64>,
                             FastrangeFunc<HASH_64>, Hashtable::BiasedKicking<26>>(ht_capacity));
   measure(Hashtable::Cuckoo<T, uint32_t, 8, Murmur3FinalizerFunc, Murmur3FinalizerCuckoo2Func, FastModuloFunc<HASH_64>,
                             FastModuloFunc<HASH_64>, Hashtable::BiasedKicking<26>>(ht_capacity));
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
         const auto dataset_elem_count = dataset_size / dataset.bytesPerValue;

         for (const auto& load_fac : args.load_factors) {
            const auto ht_capacity = static_cast<double>(dataset_elem_count) / load_fac;

            using Chained =
               Hashtable::Chained<uint64_t, uint32_t, 4, PrimeMultiplicationHash64, FastrangeFunc<HASH_64>>;
            // Directory size + all keys go to one bucket chain
            const auto wc_chaining = Chained::directory_address_count(ht_capacity) * Chained::slot_byte_size() +
               ((dataset_elem_count - 1) / Chained::bucket_size()) * Chained::bucket_byte_size();

            using Probing = Hashtable::Probing<uint64_t, uint32_t, PrimeMultiplicationHash64, FastrangeFunc<HASH_64>,
                                               Hashtable::LinearProbingFunc>;
            const auto wc_probing = Probing::bucket_byte_size() * Probing::directory_address_count(ht_capacity);

            using Cuckoo =
               Hashtable::Cuckoo<uint64_t, uint32_t, 8, PrimeMultiplicationHash64, PrimeMultiplicationHash64,
                                 FastrangeFunc<HASH_64>, FastrangeFunc<HASH_64>, Hashtable::UnbiasedKicking>;
            const auto wc_cuckoo = Cuckoo::bucket_byte_size() * Cuckoo::directory_address_count(ht_capacity);

            exec_mem.emplace_back(dataset_size + varmax(wc_chaining, wc_probing, wc_cuckoo));
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
