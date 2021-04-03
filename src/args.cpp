#include <iostream>
#include <stdexcept>
#include <string>

#include "args.hpp"
#include "convenience.hpp"
#include "dataset.hpp"

std::optional<std::string> Args::option(const int argc, const char* argv[], const std::string& option) {
   for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      auto start = arg.find(option);
      if (0 == start) {
         return arg.substr(option.length(), arg.length() - option.length());
      }
   }
   return {};
}

Args Args::parse(const int argc, const char* argv[]) {
   double over_alloc = 1.0;
   int bucket_size = 1;
   std::vector<uint64_t> dataset;
   std::string datapath;

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

   // Parse dataset parameter
   if (const auto raw_dataset = option(argc, argv, "-dataset=")) {
      const auto it = DATASETS.find(*raw_dataset);
      if (it != DATASETS.end()) {
         dataset = it->second.load(datapath);
      } else {
         throw std::runtime_error("Unknown dataset: " + *raw_dataset);
      }
   } else {
      throw std::runtime_error("Please specify a dataset using \"-dataset=<DATASET>\"");
   }

   return Args(over_alloc, bucket_size, dataset, datapath);
}