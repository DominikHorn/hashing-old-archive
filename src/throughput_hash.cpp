#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <convenience.hpp>
#include <hashing.hpp>
#include <reduction.hpp>

#include "include/args.hpp"
#include "include/benchmark.hpp"
#include "include/csv.hpp"

using Args = BenchmarkArgs::HashThroughputArgs;

template<class Hashfn, class Reducerfn, class Data>
static void
measure(const std::string& dataset_name, const std::vector<Data>& dataset, CSV& outfile, std::mutex& iomutex) {
   const auto str = [](auto s) { return std::to_string(s); };
   std::map<std::string, std::string> datapoint({
      {"dataset", dataset_name},
      {"numelements", str(dataset.size())},
      {"hash", Hashfn::name()},
      {"reducer", Reducerfn::name()},
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

   // Measure & log
   const auto stats = Benchmark::measure_throughput<Hashfn, Reducerfn>(dataset);
#ifdef VERBOSE
   {
      std::unique_lock<std::mutex> lock(iomutex);
      std::cout << std::setw(55) << std::right << Reducerfn::name() + "(" + Hashfn::name() + "): "
                << relative_to(stats.average_total_inference_reduction_ns, dataset.size()) << " ns/key on average for "
                << stats.repeatCnt << " repetitions ("
                << nanoseconds_to_seconds(stats.average_total_inference_reduction_ns) << " s total)" << std::endl;
   };
#endif

   datapoint.emplace("nanoseconds_total", str(stats.average_total_inference_reduction_ns));
   datapoint.emplace("nanoseconds_per_key",
                     str(relative_to(stats.average_total_inference_reduction_ns, dataset.size())));
   datapoint.emplace("benchmark_repeat_cnt", str(stats.repeatCnt));

   // Write to csv
   outfile.write(datapoint);
};

template<class Hashfn, class Dataset>
static void measure64(const std::string& dataset_name, const Dataset& dataset, CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;
   measure<Hashfn, DoNothing<HASH_64>>(dataset_name, dataset, outfile, iomutex);
   measure<Hashfn, Fastrange<HASH_32>>(dataset_name, dataset, outfile, iomutex);
   measure<Hashfn, Fastrange<HASH_64>>(dataset_name, dataset, outfile, iomutex);
   measure<Hashfn, Modulo<HASH_64>>(dataset_name, dataset, outfile, iomutex);
   measure<Hashfn, FastModulo<HASH_64>>(dataset_name, dataset, outfile, iomutex);
   measure<Hashfn, BranchlessFastModulo<HASH_64>>(dataset_name, dataset, outfile, iomutex);
}

template<class Hashfn, class Dataset>
static void measure128(const std::string& dataset_name, const Dataset& dataset, CSV& outfile, std::mutex& iomutex) {
   using namespace Reduction;
   measure64<Reduction::Lower<Hashfn>>(dataset_name, dataset, outfile, iomutex);
   measure64<Reduction::Higher<Hashfn>>(dataset_name, dataset, outfile, iomutex);
   measure64<Reduction::Xor<Hashfn>>(dataset_name, dataset, outfile, iomutex);
   measure64<Reduction::City<Hashfn>>(dataset_name, dataset, outfile, iomutex);
}

// TODO: ensure that we don't use two separate reducers for 128bit hashes for throughput benchmark, i.e.,
//  some 64 bit hashes are already implemented as 128bit hash + reducer!
int main(int argc, char* argv[]) {
   try {
      Args args(argc, argv);
      CSV outfile(args.outfile,
                  {
                     "dataset",
                     "numelements",
                     "hash",
                     "reducer",
                     "nanoseconds_total",
                     "nanoseconds_per_key",
                     "benchmark_repeat_cnt",
                  });

      // Worker pool for speeding up the benchmarking
      std::mutex iomutex;
      std::counting_semaphore cpu_blocker(args.max_threads);
      std::vector<std::thread> threads{};

      for (const auto& it : args.datasets) {
         threads.emplace_back(std::thread([&, it] {
            cpu_blocker.aquire();

            const auto name = it.name();
            using Data = uint64_t;
            std::vector<Data> dataset = it.load(iomutex);

            /**
            * ====================================
            *           Actual measuring
            * ====================================
            */
            measure64<PrimeMultiplicationHash64>(name, dataset, outfile, iomutex);
            measure64<FibonacciHash64>(name, dataset, outfile, iomutex);
            measure64<FibonacciPrimeHash64>(name, dataset, outfile, iomutex);
            measure64<MultAddHash64>(name, dataset, outfile, iomutex);

            // More significant bits supposedly are of higher quality for multiplicative methods -> compute
            // how much we need to shift/rotate to throw away the least/make 'high quality bits' as prominent as possible
            constexpr auto p = (sizeof(Data) * 8) - __builtin_clzll(200000000 - 1);
            measure64<PrimeMultiplicationShiftHash64<p>>(name, dataset, outfile, iomutex);
            measure64<FibonacciShiftHash64<p>>(name, dataset, outfile, iomutex);
            measure64<FibonacciPrimeShiftHash64<p>>(name, dataset, outfile, iomutex);
            measure64<MultAddShiftHash64<p>>(name, dataset, outfile, iomutex);

            // TODO: measure rotation instead of shift?
            //            const unsigned int rot = 64 - p;
            //            measure_hashfn("mult64_rotate" + std::to_string(rot),
            //                           [&](HASH_64 key) { return rotr(MultHash::mult64_hash(key), rot); });

            measure128<Murmur3Hash128<>>(name, dataset, outfile, iomutex);
            measure64<MurmurFinalizer<Data>>(name, dataset, outfile, iomutex);

            measure64<XXHash64<Data>>(name, dataset, outfile, iomutex);
            measure64<XXHash3<Data>>(name, dataset, outfile, iomutex);
            measure128<XXHash3_128<Data>>(name, dataset, outfile, iomutex);

            measure64<SmallTabulationHash<Data>>(name, dataset, outfile, iomutex);
            measure64<MediumTabulationHash<Data>>(name, dataset, outfile, iomutex);
            measure64<LargeTabulationHash<Data>>(name, dataset, outfile, iomutex);

            measure64<CityHash64<Data>>(name, dataset, outfile, iomutex);
            measure128<CityHash128<Data>>(name, dataset, outfile, iomutex);

            measure64<MeowHash64<Data, 0>>(name, dataset, outfile, iomutex);
            measure64<MeowHash64<Data, 1>>(name, dataset, outfile, iomutex);

            measure64<AquaHash<Data, 0>>(name, dataset, outfile, iomutex);
            measure64<AquaHash<Data, 1>>(name, dataset, outfile, iomutex);

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