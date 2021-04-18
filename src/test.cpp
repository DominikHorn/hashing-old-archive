#include <string>

#include <cxxopts.hpp>

#include "include/dataset.hpp"

int main(int argc, char* argv[]) {
   const std::string help_key = "help";
   const std::string outfile_key = "outfile";
   const std::string loadfactors_key = "loadfactors";
   const std::string datasets_key = "datasets";

   const std::vector<std::string> required{outfile_key, datasets_key};

   try {
      // Define
      cxxopts::Options options("Test",
                               "Simple test executable to test various components of this repository during "
                               "development. "
                               "Subject to frequent and arbitrary change");
      options.add_options()("h," + help_key, "display help") //
         (outfile_key,
          "path to output file for storing results as csv. NOTE: file will always be overwritten",
          cxxopts::value<std::string>()) //
         (loadfactors_key,
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
      const auto outfile = result[outfile_key].as<std::string>();
      const auto loadfactors = result[loadfactors_key].as<std::vector<double>>();
      const auto datasets = result[datasets_key].as<std::vector<Dataset>>();
   } catch (const std::exception& ex) {
      std::cerr << "error: " << ex.what() << std::endl;
      std::cerr << "Use --help for information on how to run this benchmark" << std::endl;
      exit(1);
   }
}