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
   "dataset", "numelements", "load_factor", "sample_size", "bucket_size", "hashtable", "model", "reducer", "payload",
   "insert_nanoseconds_total", "insert_nanoseconds_per_key", "avg_lookup_nanoseconds_total",
   "avg_lookup_nanoseconds_per_key", "median_lookup_nanoseconds_total", "median_lookup_nanoseconds_per_key",
   "unsuccessful_lookup_percent",

   // Cuckoo custom statistics
   "primary_key_ratio",

   // Chained custom statistics
   "empty_buckets", "min_chain_length", "max_chain_length", "additional_buckets", "empty_additional_slots",

   // Probing custom statistics
   "min_psl", "max_psl", "total_psl"

   //
};

template<class Data>
struct Payload16 {
   uint64_t q0 = 0, q1 = 0;
   explicit Payload16(const Data& key) : q0(key + 1), q1(key + 2) {}
   explicit Payload16() {}

   bool operator==(const Payload16& other) {
      return q0 == other.q0 && q1 == other.q1;
   }
} packed;

template<class Data>
struct Payload64 {
   uint64_t q0 = 0, q1 = 0, q2 = 0, q3 = 0, q4 = 0, q5 = 0, q6 = 0, q7 = 0;
   explicit Payload64(const Data& key)
      : q0(key - 4), q1(key - 3), q2(key - 2), q3(key - 1), q4(key + 1), q5(key + 2), q6(key + 3), q7(key + 4) {}
   explicit Payload64() {}

   bool operator==(const Payload64& other) {
      return q0 == other.q0 && q1 == other.q1 && q2 == other.q2 && q3 == other.q3 && q4 == other.q4 && q5 == other.q5 &&
         q6 == other.q6 && q7 == other.q7;
   }
} packed;

static const auto UNSUCCESSFUL_0_PERCENT = 0;
static const auto UNSUCCESSFUL_25_PERCENT = std::numeric_limits<uint32_t>::max() / 4;
static const auto UNSUCCESSFUL_50_PERCENT = UNSUCCESSFUL_25_PERCENT * 2;
static const auto UNSUCCESSFUL_75_PERCENT = UNSUCCESSFUL_25_PERCENT * 3;

template<class Hashfn, class Hashtable, const uint32_t UnsuccessfulLookupPercent = UNSUCCESSFUL_0_PERCENT, class Data>
static void measure(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                    const std::vector<Data>& sample, CSV& outfile, std::mutex& iomutex) {
   const auto str = [](auto s) { return std::to_string(s); };
   std::map<std::string, std::string> datapoint(
      {{"dataset", dataset_name},
       {"numelements", str(dataset.size())},
       {"load_factor", str(load_factor)},
       {"bucket_size", str(Hashtable::bucket_size())},
       {"hashtable", Hashtable::name()},
       {"payload", str(sizeof(typename Hashtable::PayloadType))},
       {"model", Hashtable::hash_name()},
       {"reducer", Hashtable::reducer_name()},
       {"unsuccessful_lookup_percent",
        str(relative_to(UnsuccessfulLookupPercent, std::numeric_limits<uint32_t>::max()))}});

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

   // Theoretical slot count of a hashtable on which we want to measure collisions
   const double unsuccessful_perc = relative_to(UnsuccessfulLookupPercent, std::numeric_limits<uint32_t>::max());
   const auto ht_capacity =
      static_cast<uint64_t>(static_cast<double>(dataset.size()) * unsuccessful_perc / static_cast<double>(load_factor));
   Hashtable hashtable(ht_capacity,
                       Hashfn(sample.begin(), sample.end(), Hashtable::directory_address_count(ht_capacity)));
   try {
      // Measure
      const auto stats = Benchmark::measure_hashtable<UnsuccessfulLookupPercent>(dataset, hashtable);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << std::setw(55) << std::right
                   << Hashtable::reducer_name() + "(" + Hashtable::hash_name() + ") insert took "
                   << relative_to(stats.total_insert_ns, dataset.size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.total_insert_ns) << " s total), lookup took "
                   << relative_to(stats.median_total_lookup_ns, dataset.size()) << " ns/key ("
                   << nanoseconds_to_seconds(stats.median_total_lookup_ns) << " s total)" << std::endl;
      };
#endif

      datapoint.emplace("insert_nanoseconds_total", str(stats.total_insert_ns));
      datapoint.emplace("insert_nanoseconds_per_key", str(relative_to(stats.total_insert_ns, dataset.size())));
      datapoint.emplace("avg_lookup_nanoseconds_total", str(stats.avg_total_lookup_ns));
      datapoint.emplace("avg_lookup_nanoseconds_per_key", str(relative_to(stats.avg_total_lookup_ns, dataset.size())));
      datapoint.emplace("median_lookup_nanoseconds_total", str(stats.median_total_lookup_ns));
      datapoint.emplace("median_lookup_nanoseconds_per_key",
                        str(relative_to(stats.median_total_lookup_ns, dataset.size())));

      // Make sure we collect more insight based on hashtable
      for (const auto& stat : hashtable.lookup_statistics(dataset)) {
         datapoint.emplace(stat);
      }
   } catch (const std::exception& e) {
      std::unique_lock<std::mutex> lock(iomutex);
      std::cout << std::setw(55) << std::right
                << Hashtable::reducer_name() + "(" + Hashtable::hash_name() + ") failed: " << e.what() << std::endl;
   }

   // Write to csv
   outfile.write(datapoint);
}

template<class Hashfn, class Data>
static void measure_chained(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                            const std::vector<Data>& sample, CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;
   measure<Hashfn, Hashtable::Chained<Data, Payload16<Data>, 1, Hashfn, Clamp<HASH_64>>>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
   measure<Hashfn, Hashtable::Chained<Data, Payload64<Data>, 1, Hashfn, Clamp<HASH_64>>>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);

   measure<Hashfn, Hashtable::Chained<Data, Payload16<Data>, 4, Hashfn, Clamp<HASH_64>>>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
   measure<Hashfn, Hashtable::Chained<Data, Payload64<Data>, 4, Hashfn, Clamp<HASH_64>>>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
}

template<class Hashfn, const uint32_t UnsuccessfulLookupPercent = UNSUCCESSFUL_0_PERCENT, class Data>
static void measure_probing(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                            const std::vector<Data>& sample, CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;

   /// Standard probing
   measure<Hashfn, Hashtable::Probing<Data, Payload16<Data>, Hashfn, Clamp<HASH_64>, Hashtable::LinearProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
   measure<Hashfn, Hashtable::Probing<Data, Payload64<Data>, Hashfn, Clamp<HASH_64>, Hashtable::LinearProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);

   measure<Hashfn, Hashtable::Probing<Data, Payload16<Data>, Hashfn, Clamp<HASH_64>, Hashtable::QuadraticProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
   measure<Hashfn, Hashtable::Probing<Data, Payload64<Data>, Hashfn, Clamp<HASH_64>, Hashtable::QuadraticProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);

   /// Robin Hood
   measure<Hashfn,
           Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, Clamp<HASH_64>, Hashtable::LinearProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
   measure<Hashfn,
           Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, Clamp<HASH_64>, Hashtable::LinearProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);

   measure<Hashfn,
           Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, Clamp<HASH_64>, Hashtable::QuadraticProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
   measure<Hashfn,
           Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, Clamp<HASH_64>, Hashtable::QuadraticProbingFunc>,
           UnsuccessfulLookupPercent>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
}

template<class Hashfn, class Data>
static void measure_cuckoo(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                           const std::vector<Data>& sample, CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;

   /// Balanced kicking (insert into bucket with more free space, if both are full kick with 50% chance from either)
   measure<Hashfn,
           Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn, Murmur3FinalizerCuckoo2Func, Clamp<HASH_64>,
                             FastModulo<HASH_64>, Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
   measure<Hashfn,
           Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn, Murmur3FinalizerCuckoo2Func, Clamp<HASH_64>,
                             FastModulo<HASH_64>, Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);

   /// Biased kicking (place in primary bucket first & kick from secondary bucket with 10% chance)
   measure<Hashfn,
           Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn, Murmur3FinalizerCuckoo2Func, Clamp<HASH_64>,
                             FastModulo<HASH_64>, Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor,
                                                                                 sample, outfile, iomutex);
   measure<Hashfn,
           Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn, Murmur3FinalizerCuckoo2Func, Clamp<HASH_64>,
                             FastModulo<HASH_64>, Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor,
                                                                                 sample, outfile, iomutex);
}

template<class Data>
static void benchmark(const std::string& dataset_name, const std::vector<Data>& dataset, CSV& outfile,
                      std::mutex& iomutex) {
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

      return sample;
   };

   const auto sort_prepare = [](auto& sample) { std::sort(sample.begin(), sample.end()); };

   // Take a random sample
   auto sample = pgm_sample_fn();
   sort_prepare(sample);

   /// Chained
   for (const auto load_factor : {1. / 0.75, 1. / 1., 1. / 1.25}) {
      measure_chained<rs::RadixSplineHash<Data>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);

      try {
         measure_chained<PGMHash<Data, 128, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
      } catch (const std::exception& e) {
         try {
            measure_chained<PGMHash<Data, 256, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
         } catch (const std::exception& e) {
            try {
               measure_chained<PGMHash<Data, 512, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                            iomutex);
            } catch (const std::exception& e) {
               try {
                  measure_chained<PGMHash<Data, 1024, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                                iomutex);
               } catch (const std::exception& e) {
                  std::unique_lock<std::mutex> lock(iomutex);
                  std::cerr << "Failed to find viable epsrec0 configuration for " << dataset_name << std::endl;
               }
            }
         }
      }

      try {
         measure_chained<PGMHash<Data, 128, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
      } catch (const std::exception& e) {
         try {
            measure_chained<PGMHash<Data, 256, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
         } catch (const std::exception& e) {
            try {
               measure_chained<PGMHash<Data, 512, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                            iomutex);
            } catch (const std::exception& e) {
               try {
                  measure_chained<PGMHash<Data, 1024, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                                iomutex);
               } catch (const std::exception& e) {
                  std::unique_lock<std::mutex> lock(iomutex);
                  std::cerr << "Failed to find viable epsrec0 configuration for " << dataset_name << std::endl;
               }
            }
         }
      }
   }

   /// Cuckoo
   for (const auto load_factor : {0.98, 0.95}) {
      measure_cuckoo<rs::RadixSplineHash<Data>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);

      try {
         measure_cuckoo<PGMHash<Data, 128, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
      } catch (const std::exception& e) {
         try {
            measure_cuckoo<PGMHash<Data, 256, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
         } catch (const std::exception& e) {
            try {
               measure_cuckoo<PGMHash<Data, 512, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                           iomutex);
            } catch (const std::exception& e) {
               try {
                  measure_cuckoo<PGMHash<Data, 1024, 0, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                               iomutex);
               } catch (const std::exception& e) {
                  std::unique_lock<std::mutex> lock(iomutex);
                  std::cerr << "Failed to find viable epsrec0 configuration for " << dataset_name << std::endl;
               }
            }
         }
      }

      try {
         measure_cuckoo<PGMHash<Data, 128, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
      } catch (const std::exception& e) {
         try {
            measure_cuckoo<PGMHash<Data, 256, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile, iomutex);
         } catch (const std::exception& e) {
            try {
               measure_cuckoo<PGMHash<Data, 512, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                           iomutex);
            } catch (const std::exception& e) {
               try {
                  measure_cuckoo<PGMHash<Data, 1024, 4, 1000>>(dataset_name, dataset, load_factor, sample, outfile,
                                                               iomutex);
               } catch (const std::exception& e) {
                  std::unique_lock<std::mutex> lock(iomutex);
                  std::cerr << "Failed to find viable epsrec0 configuration for " << dataset_name << std::endl;
               }
            }
         }
      }
   }

   /// Probing
   for (const auto load_factor : {1.0 / 1.25, 1.0 / 1.5}) {
      measure_probing<rs::RadixSplineHash<Data>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor, sample,
                                                                         outfile, iomutex);
      measure_probing<rs::RadixSplineHash<Data>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor, sample,
                                                                          outfile, iomutex);
      measure_probing<rs::RadixSplineHash<Data>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor, sample,
                                                                          outfile, iomutex);
      measure_probing<rs::RadixSplineHash<Data>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor, sample,
                                                                          outfile, iomutex);

      try {
         measure_probing<PGMHash<Data, 128, 0, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor,
                                                                              sample, outfile, iomutex);
         measure_probing<PGMHash<Data, 128, 0, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
         measure_probing<PGMHash<Data, 128, 0, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
         measure_probing<PGMHash<Data, 128, 0, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
      } catch (const std::exception& e) {
         try {
            measure_probing<PGMHash<Data, 256, 0, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor,
                                                                                 sample, outfile, iomutex);
            measure_probing<PGMHash<Data, 256, 0, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor,
                                                                                  sample, outfile, iomutex);
            measure_probing<PGMHash<Data, 256, 0, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor,
                                                                                  sample, outfile, iomutex);
            measure_probing<PGMHash<Data, 256, 0, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor,
                                                                                  sample, outfile, iomutex);
         } catch (const std::exception& e) {
            try {
               measure_probing<PGMHash<Data, 512, 0, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor,
                                                                                    sample, outfile, iomutex);
               measure_probing<PGMHash<Data, 512, 0, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor,
                                                                                     sample, outfile, iomutex);
               measure_probing<PGMHash<Data, 512, 0, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor,
                                                                                     sample, outfile, iomutex);
               measure_probing<PGMHash<Data, 512, 0, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor,
                                                                                     sample, outfile, iomutex);
            } catch (const std::exception& e) {
               try {
                  measure_probing<PGMHash<Data, 1024, 0, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset,
                                                                                        load_factor, sample, outfile,
                                                                                        iomutex);
                  measure_probing<PGMHash<Data, 1024, 0, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
                  measure_probing<PGMHash<Data, 1024, 0, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
                  measure_probing<PGMHash<Data, 1024, 0, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
               } catch (const std::exception& e) {
                  std::unique_lock<std::mutex> lock(iomutex);
                  std::cerr << "Failed to find viable epsrec0 configuration for " << dataset_name << std::endl;
               }
            }
         }
      }

      try {
         measure_probing<PGMHash<Data, 128, 4, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor,
                                                                              sample, outfile, iomutex);
         measure_probing<PGMHash<Data, 128, 4, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
         measure_probing<PGMHash<Data, 128, 4, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
         measure_probing<PGMHash<Data, 128, 4, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor,
                                                                               sample, outfile, iomutex);
      } catch (const std::exception& e) {
         try {
            measure_probing<PGMHash<Data, 256, 4, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor,
                                                                                 sample, outfile, iomutex);
            measure_probing<PGMHash<Data, 256, 4, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor,
                                                                                  sample, outfile, iomutex);
            measure_probing<PGMHash<Data, 256, 4, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor,
                                                                                  sample, outfile, iomutex);
            measure_probing<PGMHash<Data, 256, 4, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor,
                                                                                  sample, outfile, iomutex);
         } catch (const std::exception& e) {
            try {
               measure_probing<PGMHash<Data, 512, 4, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset, load_factor,
                                                                                    sample, outfile, iomutex);
               measure_probing<PGMHash<Data, 512, 4, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset, load_factor,
                                                                                     sample, outfile, iomutex);
               measure_probing<PGMHash<Data, 512, 4, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset, load_factor,
                                                                                     sample, outfile, iomutex);
               measure_probing<PGMHash<Data, 512, 4, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset, load_factor,
                                                                                     sample, outfile, iomutex);
            } catch (const std::exception& e) {
               try {
                  measure_probing<PGMHash<Data, 1024, 4, 1000>, UNSUCCESSFUL_0_PERCENT>(dataset_name, dataset,
                                                                                        load_factor, sample, outfile,
                                                                                        iomutex);
                  measure_probing<PGMHash<Data, 1024, 4, 1000>, UNSUCCESSFUL_25_PERCENT>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
                  measure_probing<PGMHash<Data, 1024, 4, 1000>, UNSUCCESSFUL_50_PERCENT>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
                  measure_probing<PGMHash<Data, 1024, 4, 1000>, UNSUCCESSFUL_75_PERCENT>(dataset_name, dataset,
                                                                                         load_factor, sample, outfile,
                                                                                         iomutex);
               } catch (const std::exception& e) {
                  std::unique_lock<std::mutex> lock(iomutex);
                  std::cerr << "Failed to find viable epsrec4 configuration for " << dataset_name << std::endl;
               }
            }
         }
      }
   }
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
         const auto dataset = it.load(iomutex);
         benchmark(it.name(), dataset, outfile, iomutex);
      }
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
