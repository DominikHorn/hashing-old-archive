#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/resource.h>

#include <convenience.hpp>
#include <hashtable.hpp>
#include <learned_models.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"
#include "include/functors/hash_functors.hpp"

using Args = BenchmarkArgs::LearnedHashtableArgs;

const std::vector<std::string> csv_columns = {
   // General statistics
   "dataset", "numelements", "load_factor", "sample_size", "bucket_size", "hashtable", "model", "reducer",
   "insert_nanoseconds_total", "insert_nanoseconds_per_key", "lookup_nanoseconds_total", "lookup_nanoseconds_per_key",

   // Cuckoo custom statistics
   "primary_key_ratio",

   // Chained custom statistics
   "empty_buckets", "min_chain_length", "max_chain_length", "total_chain_pointer_count",

   // Probing custom statistics
   "min_psl", "max_psl", "total_psl"

   //
};

template<class Data>
static void benchmark(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                      CSV& outfile, std::mutex& iomutex) {
   const auto measure = [&](auto hashtable, const auto& sample_size) {
      const auto hash_name = hashtable.hash_name();
      const auto reducer_name = hashtable.reducer_name();

      const auto str = [](auto s) { return std::to_string(s); };
      std::map<std::string, std::string> datapoint({
         {"dataset", dataset_name},
         {"numelements", str(dataset.size())},
         {"load_factor", str(load_factor)},
         {"sample_size", str(sample_size)},
         {"bucket_size", str(hashtable.bucket_size())},
         {"hashtable", hashtable.name()},
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
      try {
         const auto stats = Benchmark::measure_hashtable(dataset, hashtable);

#ifdef VERBOSE
         {
            std::unique_lock<std::mutex> lock(iomutex);
            std::cout << std::setw(55) << std::right << reducer_name + "(" + hash_name + ") insert took "
                      << relative_to(stats.total_insert_ns, dataset.size()) << " ns/key ("
                      << nanoseconds_to_seconds(stats.total_insert_ns) << " s total), lookup took "
                      << relative_to(stats.total_lookup_ns, dataset.size()) << " ns/key ("
                      << nanoseconds_to_seconds(stats.total_lookup_ns) << " s total)" << std::endl;
         };
#endif

         datapoint.emplace("insert_nanoseconds_total", str(stats.total_insert_ns));
         datapoint.emplace("insert_nanoseconds_per_key", str(relative_to(stats.total_insert_ns, dataset.size())));
         datapoint.emplace("lookup_nanoseconds_total", str(stats.total_lookup_ns));
         datapoint.emplace("lookup_nanoseconds_per_key", str(relative_to(stats.total_lookup_ns, dataset.size())));

         // Make sure we collect more insight based on hashtable
         for (const auto& stat : hashtable.lookup_statistics(dataset)) {
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

   /**
    * ====================================
    *        Learned Indices
    * ====================================
    */

   // Take a sample
   const double sample_size = 0.01; // Fix this for now at 0.01
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

   const auto sort_prepare = [](auto& sample) { std::sort(sample.begin(), sample.end()); };

   // Take a random sample
   auto sample = pgm_sample_fn();
   sort_prepare(sample);

   const auto ht_capacity =
      static_cast<uint64_t>(static_cast<double>(dataset.size()) / static_cast<double>(load_factor));

   /**
    * ========================================================
    *  Chained, 64-bit key, 32-bit payload, 1 slots per bucket
    * ========================================================
    */
   using namespace Reduction;
   {
      // pgm_hash_eps256_epsrec0
      using HT = Hashtable::Chained<Data, uint32_t, 1, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps128_epsrec4
      using HT = Hashtable::Chained<Data, uint32_t, 1, PGMHash<HASH_64, 128, 4>, MinMaxCutoff<HASH_64>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps64_epsrec1
      using HT = Hashtable::Chained<Data, uint32_t, 1, PGMHash<HASH_64, 64, 1>, MinMaxCutoff<HASH_64>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps4_epsrec4
      using HT = Hashtable::Chained<Data, uint32_t, 1, PGMHash<HASH_64, 4, 4>, MinMaxCutoff<HASH_64>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }

   /**
    * ========================================================
    *  Cuckoo, 64-bit key, 32-bit payload, 8 slots per bucket
    * ========================================================
    */
   /// Balanced
   {
      // pgm_hash_eps256_epsrec0
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 256, 0>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BalancedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps128_epsrec4
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 128, 4>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BalancedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps64_epsrec1
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 64, 1>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BalancedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps4_epsrec4
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 4, 4>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BalancedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }

   /// Biased (~10% secondary bucket kicking)
   {
      // pgm_hash_eps256_epsrec0
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 256, 0>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BiasedKicking<26>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps128_epsrec4
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 128, 4>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BiasedKicking<26>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps64_epsrec1
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 64, 1>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BiasedKicking<26>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps4_epsrec4
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 4, 4>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::BiasedKicking<26>>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }

   /// Unbiased (100% kick from primary bucket)
   {
      // pgm_hash_eps256_epsrec0
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 256, 0>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::UnbiasedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps128_epsrec4
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 128, 4>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::UnbiasedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps64_epsrec1
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 64, 1>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::UnbiasedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }
   {
      // pgm_hash_eps4_epsrec4
      using HT = Hashtable::Cuckoo<Data, uint32_t, 8, PGMHash<HASH_64, 4, 4>, Murmur3FinalizerCuckoo2Func,
                                   MinMaxCutoff<HASH_64>, FastModulo<HASH_64>, Hashtable::UnbiasedKicking>;
      measure(HT(ht_capacity,
                 PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), HT::directory_address_count(ht_capacity))),
              sample_size);
   }

   /**
    * ========================================================
    *           Probing, 64-bit key, 32-bit value
    * ========================================================
    */

   const auto prob_directory_address_count =
      Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>,
                         Hashtable::LinearProbingFunc>::directory_address_count(ht_capacity);

   /// Linear
   measure(
      Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>, Hashtable::LinearProbingFunc>(
         ht_capacity, PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), prob_directory_address_count)),
      sample_size);
   measure(
      Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 128, 4>, MinMaxCutoff<HASH_64>, Hashtable::LinearProbingFunc>(
         ht_capacity, PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), prob_directory_address_count)),
      sample_size);
   measure(
      Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 64, 1>, MinMaxCutoff<HASH_64>, Hashtable::LinearProbingFunc>(
         ht_capacity, PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), prob_directory_address_count)),
      sample_size);
   measure(
      Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 4, 4>, MinMaxCutoff<HASH_64>, Hashtable::LinearProbingFunc>(
         ht_capacity, PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), prob_directory_address_count)),
      sample_size);

   /// Quadratic
   measure(Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>,
                              Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), prob_directory_address_count)),
           sample_size);
   measure(Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 128, 4>, MinMaxCutoff<HASH_64>,
                              Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), prob_directory_address_count)),
           sample_size);
   measure(Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 64, 1>, MinMaxCutoff<HASH_64>,
                              Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), prob_directory_address_count)),
           sample_size);
   measure(Hashtable::Probing<Data, uint32_t, PGMHash<HASH_64, 4, 4>, MinMaxCutoff<HASH_64>,
                              Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), prob_directory_address_count)),
           sample_size);

   /**
    * ========================================================
    *          Robin Hood, 64-bit key, 32-bit value
    * ========================================================
    */

   const auto robin_directory_address_count =
      Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>,
                                  Hashtable::LinearProbingFunc>::directory_address_count(ht_capacity);

   /// Linear
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>,
                                       Hashtable::LinearProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 128, 4>, MinMaxCutoff<HASH_64>,
                                       Hashtable::LinearProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 64, 1>, MinMaxCutoff<HASH_64>,
                                       Hashtable::LinearProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 4, 4>, MinMaxCutoff<HASH_64>,
                                       Hashtable::LinearProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);

   /// Quadratic
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 256, 0>, MinMaxCutoff<HASH_64>,
                                       Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 256, 0>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 128, 4>, MinMaxCutoff<HASH_64>,
                                       Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 128, 4>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 64, 1>, MinMaxCutoff<HASH_64>,
                                       Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 64, 1>(sample.begin(), sample.end(), robin_directory_address_count)),
           sample_size);
   measure(Hashtable::RobinhoodProbing<Data, uint32_t, PGMHash<HASH_64, 4, 4>, MinMaxCutoff<HASH_64>,
                                       Hashtable::QuadraticProbingFunc>(
              ht_capacity, PGMHash<HASH_64, 4, 4>(sample.begin(), sample.end(), robin_directory_address_count)),
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
         const auto dataset_elem_count = dataset_size / dataset.bytesPerValue;

         for (const auto& load_fac : args.load_factors) {
            const auto ht_capacity = static_cast<double>(dataset_elem_count) / load_fac;

            using Chained =
               Hashtable::Chained<uint64_t, uint32_t, 4, PrimeMultiplicationHash64, Reduction::Fastrange<HASH_64>>;
            // Directory size + worst case (all keys go to one bucket chain)
            const auto wc_chaining = Chained::directory_address_count(ht_capacity) * Chained::slot_byte_size() +
               ((dataset_elem_count - 1) / Chained::bucket_size()) * Chained::bucket_byte_size();

            using Probing = Hashtable::Probing<uint64_t, uint32_t, PrimeMultiplicationHash64,
                                               Reduction::Fastrange<HASH_64>, Hashtable::LinearProbingFunc>;
            const auto wc_probing = Probing::bucket_byte_size() * Probing::directory_address_count(ht_capacity);

            using Cuckoo = Hashtable::Cuckoo<uint64_t, uint32_t, 8, PrimeMultiplicationHash64,
                                             PrimeMultiplicationHash64, Reduction::Fastrange<HASH_64>,
                                             Reduction::Fastrange<HASH_64>, Hashtable::BalancedKicking>;
            const auto wc_cuckoo = Cuckoo::bucket_byte_size() * Cuckoo::directory_address_count(ht_capacity);

            const auto ht_worstcase_size = varmax(wc_chaining, wc_probing, wc_cuckoo);

            //            const auto learned_index_size = (?)
            //            const auto sample_size = (?)

            // Chained hashtable memory consumption upper estimate
            exec_mem.emplace_back(dataset_size + ht_worstcase_size);
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
            benchmark(it.name(), *dataset_ptr, load_factor, outfile, iomutex);
         }
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
