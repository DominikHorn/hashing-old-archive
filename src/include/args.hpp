#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "dataset.hpp"

/**
 * Parsed command line args of the application
 */
struct Args {
  public:
   const std::string datapath;
   const std::string outfile;
   const std::vector<double> load_factors;
   const std::vector<Dataset> datasets;

   /**
    * Parses the given set of command line args
    */
   static Args parse(const int argc, const char* argv[]) {
      std::string datapath;
      std::string outfile;
      std::vector<double> load_factors{};
      std::vector<Dataset> datasets{};

      // Parse load_factors parameter
      if (const auto raw_load_factors = option(argc, argv, "-loadfactors=")) {
         size_t end = 0;
         std::string s = *raw_load_factors;
         const std::string delimiter = ",";
         while (s.length() > 0) {
            end = s.find(delimiter);
            size_t next_start = end + delimiter.length();
            if (end == std::string::npos) {
               end = s.length();
               next_start = end;
            };

            load_factors.push_back(std::stod(s.substr(0, end)));

            s = s.substr(next_start);
         }
      } else {
         std::cout << "Using default load factor \"1.0\". To specify different loadfactors, use \"-loadfactors=<COMMA "
                      "SEPARATED DOUBLES (PERCENT)>\""
                   << std::endl;
         load_factors.push_back(1.0);
      }

      // Parse datasets parameter
      if (const auto raw_datasets = option(argc, argv, "-datasets=")) {
         size_t end = 0;
         std::string s = *raw_datasets;
         const std::string delimiter = ",";
         while (s.length() > 0) {
            end = s.find(delimiter);
            size_t next_start = end + delimiter.length();
            if (end == std::string::npos) {
               end = s.length();
               next_start = end;
            };

            size_t colon_pos = s.find(":");
            if (colon_pos == std::string::npos) {
               throw std::runtime_error("Wrong datasets format. Datasets are comma separated entries of the form "
                                        "<FILENAME:BYTES_PER_ENTRY>");
            }
            const auto filename = s.substr(0, colon_pos);
            const size_t bytesPerValue = std::stoi(s.substr(colon_pos + 1, s.length()));
            datasets.push_back({.filename = filename, .bytesPerValue = bytesPerValue});

            s = s.substr(next_start);
         }
      } else {
         throw std::runtime_error("Please specify datasets to benchmark using \"-datasets=<COMMA SEPARATED "
                                  "FILENAME:BYTES_PER_ENTRY>\"");
      }

      // Parse datapath parameter
      if (const auto raw_datapath = option(argc, argv, "-datapath=")) {
         datapath = *raw_datapath;
      } else {
         throw std::runtime_error("Please specify the path to the data folder using \"-datapath=<PATH_TO_DATA>\"");
      }

      // Parse outfile parameter
      if (const auto raw_outfile = option(argc, argv, "-outfile=")) {
         outfile = *raw_outfile;
      } else {
         throw std::runtime_error("Please specify the path the output csv file \"-outfile=<PATH_TO_DATA>\"");
      }

      return Args(datapath, outfile, load_factors, datasets);
   }

  private:
   static std::optional<std::string> option(const int argc, const char* argv[], const std::string& option) {
      for (int i = 0; i < argc; i++) {
         std::string arg = argv[i];
         auto start = arg.find(option);
         if (0 == start) {
            return arg.substr(option.length(), arg.length() - option.length());
         }
      }
      return {};
   }
};