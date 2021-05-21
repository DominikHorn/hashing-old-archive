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

using Args = BenchmarkArgs::HashHashtableArgs;

const std::vector<std::string> csv_columns = {
   // General statistics
   "dataset", "numelements", "load_factor", "bucket_size", "hashtable", "hash", "reducer", "payload",
   "insert_nanoseconds_total", "insert_nanoseconds_per_key", "avg_lookup_nanoseconds_total",
   "avg_lookup_nanoseconds_per_key", "median_lookup_nanoseconds_total", "median_lookup_nanoseconds_per_key",

   // Cuckoo custom statistics
   "primary_key_ratio",

   // Chained custom statistics
   "empty_buckets", "min_chain_length", "max_chain_length", "additional_buckets", "empty_additional_slots",

   // Probing custom statistics
   "min_psl", "max_psl", "total_psl"

   //
};

template<class Hashtable, class Data>
static void measure(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                    CSV& outfile, std::mutex& iomutex) {
   const auto str = [](auto s) { return std::to_string(s); };
   std::map<std::string, std::string> datapoint({
      {"dataset", dataset_name},
      {"numelements", str(dataset.size())},
      {"load_factor", str(load_factor)},
      {"bucket_size", str(Hashtable::bucket_size())},
      {"hashtable", Hashtable::name()},
      {"payload", str(sizeof(typename Hashtable::PayloadType))},
      {"hash", Hashtable::hash_name()},
      {"reducer", Hashtable::reducer_name()},
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
      // Theoretical slot count of a hashtable on which we want to measure collisions
      const auto ht_capacity =
         static_cast<uint64_t>(static_cast<double>(dataset.size()) / static_cast<double>(load_factor));
      Hashtable hashtable(ht_capacity);

      // Measure
      const auto stats = Benchmark::measure_hashtable(dataset, hashtable);

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

   // Write to csv (if experiment failed this will visibly log that)
   outfile.write(datapoint);
}

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

template<class Hashfn, class Data>
static void measure_chained(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                            CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;
   measure<Hashtable::Chained<Data, Payload16<Data>, 1, Hashfn, Fastrange<HASH_32>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload16<Data>, 1, Hashfn, Fastrange<HASH_64>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload16<Data>, 1, Hashfn, FastModulo<HASH_64>>>(dataset_name, dataset,
                                                                                      load_factor, outfile, iomutex);

   measure<Hashtable::Chained<Data, Payload16<Data>, 4, Hashfn, Fastrange<HASH_32>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload16<Data>, 4, Hashfn, Fastrange<HASH_64>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload16<Data>, 4, Hashfn, FastModulo<HASH_64>>>(dataset_name, dataset,
                                                                                      load_factor, outfile, iomutex);

   measure<Hashtable::Chained<Data, Payload64<Data>, 1, Hashfn, Fastrange<HASH_32>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload64<Data>, 1, Hashfn, Fastrange<HASH_64>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload64<Data>, 1, Hashfn, FastModulo<HASH_64>>>(dataset_name, dataset,
                                                                                      load_factor, outfile, iomutex);

   measure<Hashtable::Chained<Data, Payload64<Data>, 4, Hashfn, Fastrange<HASH_32>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload64<Data>, 4, Hashfn, Fastrange<HASH_64>>>(dataset_name, dataset, load_factor,
                                                                                     outfile, iomutex);
   measure<Hashtable::Chained<Data, Payload64<Data>, 4, Hashfn, FastModulo<HASH_64>>>(dataset_name, dataset,
                                                                                      load_factor, outfile, iomutex);
}

template<class Hashfn, class Data>
static void measure_probing(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                            CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;

   /// Standard probing
   //   measure<Hashtable::Probing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Probing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Probing<Data, Payload16<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::LinearProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Probing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Probing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Probing<Data, Payload64<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::LinearProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);

   //   measure<Hashtable::Probing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Probing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Probing<Data, Payload16<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::QuadraticProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Probing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Probing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Probing<Data, Payload64<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::QuadraticProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);

   /// Robin Hood
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<
      Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::LinearProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::LinearProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<
      Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::LinearProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);

   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<
      Hashtable::RobinhoodProbing<Data, Payload16<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::QuadraticProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_32>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<
   //      Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, Fastrange<HASH_64>, Hashtable::QuadraticProbingFunc>>(
   //      dataset_name, dataset, load_factor, outfile, iomutex);
   measure<
      Hashtable::RobinhoodProbing<Data, Payload64<Data>, Hashfn, FastModulo<HASH_64>, Hashtable::QuadraticProbingFunc>>(
      dataset_name, dataset, load_factor, outfile, iomutex);
}

template<class Hashfn1, class Hashfn2, class Data>
static void measure_cuckoo(const std::string& dataset_name, const std::vector<Data>& dataset, const double load_factor,
                           CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;

   /// Balanced kicking (insert into bucket with more free space, if both are full kick with 50% chance from either)
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_32>, Fastrange<HASH_32>,
   //                             Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_32>, Fastrange<HASH_32>,
   //                             Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_64>, Fastrange<HASH_64>,
   //                             Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_64>, Fastrange<HASH_64>,
   //                             Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, FastModulo<HASH_64>, FastModulo<HASH_64>,
                             Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, FastModulo<HASH_64>, FastModulo<HASH_64>,
                             Hashtable::BalancedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);

   /// Unbiased kicking (place in primary bucket first & always kick from primary bucket)
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_32>, Fastrange<HASH_32>,
   //                             Hashtable::UnbiasedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_32>, Fastrange<HASH_32>,
   //                             Hashtable::UnbiasedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_64>, Fastrange<HASH_64>,
   //                             Hashtable::UnbiasedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_64>, Fastrange<HASH_64>,
   //                             Hashtable::UnbiasedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, FastModulo<HASH_64>, FastModulo<HASH_64>,
   //                             Hashtable::UnbiasedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, FastModulo<HASH_64>, FastModulo<HASH_64>,
   //                             Hashtable::UnbiasedKicking>>(dataset_name, dataset, load_factor, outfile, iomutex);

   /// Biased kicking (place in primary bucket first & kick from secondary bucket with 10% chance)
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_32>, Fastrange<HASH_32>,
   //                             Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_32>, Fastrange<HASH_32>,
   //                             Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_64>, Fastrange<HASH_64>,
   //                             Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor, outfile, iomutex);
   //   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, Fastrange<HASH_64>, Fastrange<HASH_64>,
   //                             Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Cuckoo<Data, Payload16<Data>, 8, Hashfn1, Hashfn2, FastModulo<HASH_64>, FastModulo<HASH_64>,
                             Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor, outfile, iomutex);
   measure<Hashtable::Cuckoo<Data, Payload64<Data>, 8, Hashfn1, Hashfn2, FastModulo<HASH_64>, FastModulo<HASH_64>,
                             Hashtable::BiasedKicking<10>>>(dataset_name, dataset, load_factor, outfile, iomutex);
}

template<class Data>
static void benchmark(const std::string& dataset_name, const std::vector<Data>& dataset, CSV& outfile,
                      std::mutex& iomutex) {
   using namespace Reduction;

   /// Chained
   for (const auto load_factor : {1. / 0.75, 1. / 1., 1. / 1.25}) {
      measure_chained<AquaHash<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_chained<MeowHash64<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_chained<CityHash64<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_chained<LargeTabulationHash<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_chained<MurmurFinalizer<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_chained<PrimeMultiplicationHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_chained<MultAddHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_chained<FibonacciHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_chained<XXHash3<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
   }

   /// Cuckoo
   for (const auto load_factor : {0.98, 0.95}) {
      measure_cuckoo<AquaHash<Data>, Murmur3FinalizerCuckoo2Func>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_cuckoo<MeowHash64<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_cuckoo<CityHash64<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_cuckoo<LargeTabulationHash<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_cuckoo<MurmurFinalizer<Data>, Murmur3FinalizerCuckoo2Func>(dataset_name, dataset, load_factor, outfile,
                                                                         iomutex);
      measure_cuckoo<PrimeMultiplicationHash64, Murmur3FinalizerCuckoo2Func>(dataset_name, dataset, load_factor,
                                                                             outfile, iomutex);
      measure_cuckoo<MultAddHash64, Murmur3FinalizerCuckoo2Func>(dataset_name, dataset, load_factor, outfile, iomutex);
      //   measure_cuckoo<FibonacciHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_cuckoo<XXHash3<Data>, Murmur3FinalizerCuckoo2Func>(dataset_name, dataset, load_factor, outfile, iomutex);
   }

   /// Probing
   for (const auto load_factor : {1.0 / 1.25, 1.0 / 1.5}) {
      measure_probing<AquaHash<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //      measure_probing<MeowHash64<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //      measure_probing<CityHash64<t sData>>(dataset_name, dataset, load_factor, outfile, iomutex);
      //      measure_probing<LargeTabulationHash<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_probing<MurmurFinalizer<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_probing<PrimeMultiplicationHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_probing<MultAddHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      //      measure_probing<FibonacciHash64>(dataset_name, dataset, load_factor, outfile, iomutex);
      measure_probing<XXHash3<Data>>(dataset_name, dataset, load_factor, outfile, iomutex);
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
            // Directory size + all keys go to one bucket chain
            const auto wc_chaining = Chained::directory_address_count(ht_capacity) * Chained::slot_byte_size() +
               ((dataset_elem_count - 1) / Chained::bucket_size()) * Chained::bucket_byte_size();

            using Probing = Hashtable::Probing<uint64_t, uint32_t, PrimeMultiplicationHash64,
                                               Reduction::Fastrange<HASH_64>, Hashtable::LinearProbingFunc>;
            const auto wc_probing = Probing::bucket_byte_size() * Probing::directory_address_count(ht_capacity);

            using Cuckoo = Hashtable::Cuckoo<uint64_t, uint32_t, 8, PrimeMultiplicationHash64,
                                             PrimeMultiplicationHash64, Reduction::Fastrange<HASH_64>,
                                             Reduction::Fastrange<HASH_64>, Hashtable::UnbiasedKicking>;
            const auto wc_cuckoo = Cuckoo::bucket_byte_size() * Cuckoo::directory_address_count(ht_capacity);

            exec_mem.emplace_back(dataset_size + varmax(wc_chaining, wc_probing, wc_cuckoo));
         }
      }

      std::sort(exec_mem.rbegin(), exec_mem.rend());

      for (size_t i = 0; i < args.max_threads && i < exec_mem.size(); i++) {
         max_bytes += exec_mem[i];
      }

      std::cout << "Will consume max <= " << max_bytes / (std::pow(1024, 3)) << " GB of ram" << std::endl;
#endif

      CSV outfile(args.outfile, csv_columns);

      std::mutex iomutex;
      //      std::counting_semaphore cpu_blocker(args.max_threads, 0);
      //      std::vector<std::thread> threads{};

      for (const auto& it : args.datasets) {
         const auto dataset = it.load(iomutex);
         //            threads.emplace_back(std::thread([&, it, dataset, load_factor] {
         //               cpu_blocker.aquire();
         benchmark(it.name(), dataset, outfile, iomutex);
         //               cpu_blocker.release();
         //            }));
      }
      //      cpu_blocker.release(args.max_threads);
      //      for (auto& t : threads) {
      //         t.join();
      //      }
      //      threads.clear();
   } catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
   }

   return 0;
}
