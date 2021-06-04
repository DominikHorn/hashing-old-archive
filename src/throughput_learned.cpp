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
                                              "model_count",
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

template<class Hashfn, class Reducerfn, class Data>
static void measure(const std::string& dataset_name, const std::vector<Data>& dataset, const std::vector<Data>& sample,
                    const double& sample_size, const uint64_t& sample_ns, const uint64_t& prepare_ns, CSV& outfile,
                    std::mutex& iomutex) {
   const size_t N = dataset.size();

   const auto str = [](auto s) { return std::to_string(s); };
   std::map<std::string, std::string> datapoint({{"dataset", dataset_name},
                                                 {"numelements", str(dataset.size())},
                                                 {"model", Hashfn::name()},
                                                 {"reducer", Reducerfn::name()},
                                                 {"sample_size", str(sample_size)}});

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

   // Build the model (e2e time)
   auto start_time = std::chrono::steady_clock::now();
   Hashfn hashfn(sample.begin(), sample.end(), N);
   uint64_t build_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());

   // Measure throughput
   const auto stats = Benchmark::measure_throughput<Hashfn, Reducerfn>(dataset, hashfn);

   // Sum up for easier access
   const auto total_ns = sample_ns + prepare_ns + build_ns + stats.average_total_inference_reduction_ns;

#ifdef VERBOSE
   {
      std::unique_lock<std::mutex> lock(iomutex);
      std::cout << std::setw(55) << std::right << Reducerfn::name() + "(" + Hashfn::name() + ") took "
                << relative_to(total_ns, dataset.size()) << " ns/key (" << nanoseconds_to_seconds(total_ns)
                << " s total)" << std::endl;
   };
#endif

   datapoint.emplace("model_count", str(hashfn.model_count()));
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
}

template<class Data>
static void benchmark(const std::string& dataset_name, const std::vector<Data>& dataset, const double sample_chance,
                      CSV& outfile, std::mutex& iomutex) {
   uint64_t sample_ns = 0, prepare_ns = 0;

   // Take a random sample
   std::vector<Data> sample;
   {
      auto start_time = std::chrono::steady_clock::now();
      if (sample_chance == 1.0) {
         sample = dataset;
      } else {
         sample.reserve(sample_chance * dataset.size());
         std::random_device seed_gen;
         std::default_random_engine gen(seed_gen());
         std::uniform_real_distribution<double> dist(0, 1);

         for (size_t i = 0; i < dataset.size(); i++)
            if (dist(gen) < sample_chance)
               sample.push_back(dataset[i]);
      }
      sample_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());
   }

   // Prepare the sample
   {
      auto start_time = std::chrono::steady_clock::now();
      std::sort(sample.begin(), sample.end());
      prepare_ns = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time).count());
   }

   // TODO: enable rmi once implemented

   //   /// RMI
   //   measure<rmi::RMIHash<Data, 100>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                              outfile, iomutex);
   //   measure<rmi::RMIHash<Data, 1000>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                               outfile, iomutex);
   //   measure<rmi::RMIHash<Data, 10000>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                                outfile, iomutex);

   /// RadixSpline
   measure<rs::RadixSplineHash<Data, 8, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                      sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 12, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 16, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 18, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 20, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);

   measure<rs::RadixSplineHash<Data, 8, 8>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                      sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 12, 8>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 16, 8>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 18, 8>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 20, 8>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);

   measure<rs::RadixSplineHash<Data, 8, 16>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 12, 16>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 16, 16>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 18, 16>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 20, 16>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);

   measure<rs::RadixSplineHash<Data, 8, 32>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                       sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 12, 32>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 16, 32>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 18, 32>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);
   measure<rs::RadixSplineHash<Data, 20, 32>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance,
                                                                        sample_ns, prepare_ns, outfile, iomutex);

   //   /// PGM (eps_rec 4)
   //   measure<PGMHash<Data, 256, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                            outfile, iomutex);
   //   measure<PGMHash<Data, 128, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                            outfile, iomutex);
   //   measure<PGMHash<Data, 64, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                           outfile, iomutex);
   //   measure<PGMHash<Data, 16, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                           outfile, iomutex);
   //   measure<PGMHash<Data, 4, 4>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns, outfile,
   //                                                          iomutex);
   //   /// PGM (eps_rec 1)
   //   measure<PGMHash<Data, 256, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                            outfile, iomutex);
   //   measure<PGMHash<Data, 128, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                            outfile, iomutex);
   //   measure<PGMHash<Data, 64, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                           outfile, iomutex);
   //   measure<PGMHash<Data, 16, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                           outfile, iomutex);
   //   measure<PGMHash<Data, 4, 1>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns, outfile,
   //                                                          iomutex);
   //   /// PGM (eps_rec 0)
   //   measure<PGMHash<Data, 256, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                            outfile, iomutex);
   //   measure<PGMHash<Data, 128, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                            outfile, iomutex);
   //   measure<PGMHash<Data, 64, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                           outfile, iomutex);
   //   measure<PGMHash<Data, 16, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns,
   //                                                           outfile, iomutex);
   //   measure<PGMHash<Data, 4, 0>, Reduction::Clamp<size_t>>(dataset_name, dataset, sample, sample_chance, sample_ns, prepare_ns, outfile,
   //                                                          iomutex);
}

int main(int argc, char* argv[]) {
   try {
      Args args(argc, argv);
#if VERBOSE
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

      // Highest consuming thread first
      std::sort(thread_mem.rbegin(), thread_mem.rend());

      for (size_t i = 0; i < max_thread_count; i++) {
         max_bytes += thread_mem[i];
      }

      std::cout << "Will concurrently schedule <= " << max_thread_count
                << " threads while consuming <= " << static_cast<long double>(max_bytes / (std::pow(1024, 3)))
                << " GB of ram" << std::endl;
#endif

      CSV outfile(args.outfile, csv_columns);

      // Worker pool for speeding up the benchmarking
      std::mutex iomutex;
      std_ext::counting_semaphore cpu_blocker(args.max_threads);
      std::vector<std::thread> threads{};

      for (const auto& it : args.datasets) {
         for (const auto sample_size : args.sample_sizes) {
            threads.emplace_back(std::thread([&, it, sample_size] {
               cpu_blocker.aquire();

               auto dataset = it.load(iomutex);
               benchmark(it.name(), dataset, sample_size, outfile, iomutex);

               cpu_blocker.release();
            }));
         }
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
