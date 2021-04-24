#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <cxxopts.hpp>

#include "dataset.hpp"

namespace BenchmarkArgs {
   const std::string help_key = "help";
   const std::string outfile_key = "outfile";
   const std::string max_threads_key = "max_threads";
   const std::string load_factors_key = "load_factors";
   const std::string sample_sizes_key = "sample_sizes";
   const std::string datasets_key = "datasets";

   struct HashCollisionArgs {
      std::string outfile;
      unsigned int max_threads;
      std::vector<double> load_factors;
      std::vector<Dataset> datasets;

      HashCollisionArgs(int argc, char* argv[]) {
         const std::vector<std::string> required{outfile_key, datasets_key};

         try {
            // Define
            cxxopts::Options options("Hash Collision",
                                     "Benchmark designed to measure collision statistics for various hash "
                                     "functions.");
            options.add_options()("h," + help_key, "display help") //
               (outfile_key,
                "path to output file for storing results as csv. NOTE: file will always be overwritten",
                cxxopts::value<std::string>()) //
               (max_threads_key,
                "maximum amount of threads to concurrently execute. NOTE: more threads may be created but only " +
                   max_threads_key + " will actually execute at the same time.",
                cxxopts::value<unsigned int>()->default_value(std::to_string(std::thread::hardware_concurrency()))) //
               (load_factors_key,
                "comma separated list of load factors, i.e., percentage floating point values",
                cxxopts::value<std::vector<double>>()->default_value("1.0")) //
               (datasets_key,
                "datasets to benchmark on, formatted as '<PATH_TO_DATASET>:<BYTES_PER_NUMBER>'. Collects positional "
                "arguments",
                cxxopts::value<std::vector<Dataset>>());
            options.parse_positional({datasets_key});

            if (argc <= 1) {
               std::cout << options.help() << std::endl;
               exit(0);
            }

            // Parse
            auto result = options.parse(argc, argv);

            // Validate
            if (result.count(help_key)) {
               std::cout << options.help() << std::endl;
               exit(0);
            }
            for (const auto& key : required) {
               if (!result.count(key)) {
                  throw std::runtime_error("Please specify the required '" + key + "' option");
               }
            }

            // Extract
            outfile = result[outfile_key].as<std::string>();
            max_threads = result[max_threads_key].as<unsigned int>();
            load_factors = result[load_factors_key].as<std::vector<double>>();
            datasets = result[datasets_key].as<std::vector<Dataset>>();
         } catch (const std::exception& ex) {
            std::cerr << "error: " << ex.what() << std::endl;
            std::cerr << "Use --help for information on how to run this benchmark" << std::endl;
            exit(1);
         }
      }
   };

   struct LearnedCollisionArgs {
      std::string outfile;
      unsigned int max_threads;
      std::vector<double> load_factors;
      std::vector<double> sample_sizes;
      std::vector<Dataset> datasets;

      LearnedCollisionArgs(int argc, char* argv[]) {
         const std::vector<std::string> required{outfile_key, datasets_key};

         try {
            // Define
            cxxopts::Options options("Learned Collision",
                                     "Benchmark designed to measure collision statistics for learned hash functions.");
            options.add_options()("h," + help_key, "display help") //
               (outfile_key,
                "path to output file for storing results as csv. NOTE: file will always be overwritten",
                cxxopts::value<std::string>()) //
               (max_threads_key,
                "maximum amount of threads to concurrently execute. NOTE: more threads may be created but only " +
                   max_threads_key + " will actually execute at the same time.",
                cxxopts::value<unsigned int>()->default_value(std::to_string(std::thread::hardware_concurrency()))) //
               (load_factors_key,
                "comma separated list of load factors to measure, i.e., percentage floating point values",
                cxxopts::value<std::vector<double>>()->default_value("1.0")) //
               (sample_sizes_key,
                "comma separated list of sample sizes to measure, i.e., percentage floating point values",
                cxxopts::value<std::vector<double>>()->default_value("0.01")) //
               (datasets_key,
                "datasets to benchmark on, formatted as '<PATH_TO_DATASET>:<BYTES_PER_NUMBER>'. Collects positional "
                "arguments",
                cxxopts::value<std::vector<Dataset>>());
            options.parse_positional({datasets_key});

            if (argc <= 1) {
               std::cout << options.help() << std::endl;
               exit(0);
            }

            // Parse
            auto result = options.parse(argc, argv);

            // Validate
            if (result.count(help_key)) {
               std::cout << options.help() << std::endl;
               exit(0);
            }
            for (const auto& key : required) {
               if (!result.count(key)) {
                  throw std::runtime_error("Please specify the required '" + key + "' option");
               }
            }

            // Extract
            outfile = result[outfile_key].as<std::string>();
            max_threads = result[max_threads_key].as<unsigned int>();
            load_factors = result[load_factors_key].as<std::vector<double>>();
            sample_sizes = result[sample_sizes_key].as<std::vector<double>>();
            datasets = result[datasets_key].as<std::vector<Dataset>>();
         } catch (const std::exception& ex) {
            std::cerr << "error: " << ex.what() << std::endl;
            std::cerr << "Use --help for information on how to run this benchmark" << std::endl;
            exit(1);
         }
      }
   };

   struct HashThroughputArgs {
      std::string outfile;
      unsigned int max_threads;
      std::vector<Dataset> datasets;

      HashThroughputArgs(int argc, char* argv[]) {
         const std::vector<std::string> required{outfile_key, datasets_key};

         try {
            // Define
            cxxopts::Options options("Throughput",
                                     "Benchmark designed to measure throughput statistics for various hash functions.");
            options.add_options()("h," + help_key, "display help") //
               (outfile_key,
                "path to output file for storing results as csv. NOTE: file will always be overwritten",
                cxxopts::value<std::string>()) //
               (max_threads_key,
                "maximum amount of threads to concurrently execute. NOTE: more threads may be created but only " +
                   max_threads_key + " will actually execute at the same time.",
                cxxopts::value<unsigned int>()->default_value(std::to_string(std::thread::hardware_concurrency()))) //
               (datasets_key,
                "datasets to benchmark on, formatted as '<PATH_TO_DATASET>:<BYTES_PER_NUMBER>'. Collects positional "
                "arguments",
                cxxopts::value<std::vector<Dataset>>());
            options.parse_positional({datasets_key});

            if (argc <= 1) {
               std::cout << options.help() << std::endl;
               exit(0);
            }

            // Parse
            auto result = options.parse(argc, argv);

            // Validate
            if (result.count(help_key)) {
               std::cout << options.help() << std::endl;
               exit(0);
            }
            for (const auto& key : required) {
               if (!result.count(key)) {
                  throw std::runtime_error("Please specify the required '" + key + "' option");
               }
            }

            // Extract
            outfile = result[outfile_key].as<std::string>();
            max_threads = result[max_threads_key].as<unsigned int>();
            datasets = result[datasets_key].as<std::vector<Dataset>>();
         } catch (const std::exception& ex) {
            std::cerr << "error: " << ex.what() << std::endl;
            std::cerr << "Use --help for information on how to run this benchmark" << std::endl;
            exit(1);
         }
      }
   };

   struct LearnedThroughputArgs {
      std::string outfile;
      std::vector<Dataset> datasets;
      std::vector<double> sample_sizes;

      LearnedThroughputArgs(int argc, char* argv[]) {
         const std::vector<std::string> required{outfile_key, datasets_key};

         try {
            // Define
            cxxopts::Options options("Throughput",
                                     "Benchmark designed to measure throughput statistics for various hash functions.");
            options.add_options()("h," + help_key, "display help") //
               (outfile_key,
                "path to output file for storing results as csv. NOTE: file will always be overwritten",
                cxxopts::value<std::string>()) //
               (sample_sizes_key,
                "comma separated list of sample sizes to measure, i.e., percentage floating point values",
                cxxopts::value<std::vector<double>>()->default_value("0.01")) //
               (datasets_key,
                "datasets to benchmark on, formatted as '<PATH_TO_DATASET>:<BYTES_PER_NUMBER>'. Collects positional "
                "arguments",
                cxxopts::value<std::vector<Dataset>>());
            options.parse_positional({datasets_key});

            if (argc <= 1) {
               std::cout << options.help() << std::endl;
               exit(0);
            }

            // Parse
            auto result = options.parse(argc, argv);

            // Validate
            if (result.count(help_key)) {
               std::cout << options.help() << std::endl;
               exit(0);
            }
            for (const auto& key : required) {
               if (!result.count(key)) {
                  throw std::runtime_error("Please specify the required '" + key + "' option");
               }
            }

            // Extract
            outfile = result[outfile_key].as<std::string>();
            sample_sizes = result[sample_sizes_key].as<std::vector<double>>();
            datasets = result[datasets_key].as<std::vector<Dataset>>();
         } catch (const std::exception& ex) {
            std::cerr << "error: " << ex.what() << std::endl;
            std::cerr << "Use --help for information on how to run this benchmark" << std::endl;
            exit(1);
         }
      }
   };

   struct HashHashtableArgs {
      std::string outfile;
      std::vector<double> load_factors;
      std::vector<Dataset> datasets;

      HashHashtableArgs(int argc, char* argv[]) {
         const std::vector<std::string> required{outfile_key, datasets_key};

         try {
            // Define
            cxxopts::Options options("Throughput",
                                     "Benchmark designed to measure throughput statistics for various hash functions.");
            options.add_options()("h," + help_key, "display help") //
               (outfile_key,
                "path to output file for storing results as csv. NOTE: file will always be overwritten",
                cxxopts::value<std::string>()) //
               (load_factors_key,
                "comma separated list of load factors, i.e., percentage floating point values",
                cxxopts::value<std::vector<double>>()->default_value("1.0")) //
               (datasets_key,
                "datasets to benchmark on, formatted as '<PATH_TO_DATASET>:<BYTES_PER_NUMBER>'. Collects positional "
                "arguments",
                cxxopts::value<std::vector<Dataset>>());
            options.parse_positional({datasets_key});

            if (argc <= 1) {
               std::cout << options.help() << std::endl;
               exit(0);
            }

            // Parse
            auto result = options.parse(argc, argv);

            // Validate
            if (result.count(help_key)) {
               std::cout << options.help() << std::endl;
               exit(0);
            }
            for (const auto& key : required) {
               if (!result.count(key)) {
                  throw std::runtime_error("Please specify the required '" + key + "' option");
               }
            }

            // Extract
            outfile = result[outfile_key].as<std::string>();
            load_factors = result[load_factors_key].as<std::vector<double>>();
            datasets = result[datasets_key].as<std::vector<Dataset>>();
         } catch (const std::exception& ex) {
            std::cerr << "error: " << ex.what() << std::endl;
            std::cerr << "Use --help for information on how to run this benchmark" << std::endl;
            exit(1);
         }
      }
   };
} // namespace BenchmarkArgs