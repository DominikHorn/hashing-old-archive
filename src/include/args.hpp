#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * Parsed command line args of the application
 */
struct Args {
  public:
   const double over_alloc;
   const int bucket_size;
   const std::string datapath;
   const std::string outfile;

   /**
    * Parses the given set of command line args
    */
   static Args parse(const int argc, const char* argv[]) {
      double over_alloc = 1.0;
      int bucket_size = 1;
      std::string datapath;
      std::string outfile;

      // Parse over_alloc parameter
      if (const auto raw_over_alloc = option(argc, argv, "-over-alloc=")) {
         over_alloc = std::stod(*raw_over_alloc);
      }

      // Parse bucket_size parameter
      if (const auto raw_bucket_size = option(argc, argv, "-bucket-size=")) {
         bucket_size = std::stoi(*raw_bucket_size);
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

      return Args(over_alloc, bucket_size, datapath, outfile);
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