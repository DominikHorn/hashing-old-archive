#include <iostream>
#include <string>

#include "args.hpp"
#include "convenience.hpp"

struct RawDataset {
   /// file name of the dataset
   std::string file;

   /// Bytes per value, i.e., 4 for 32-bit integers, 8 for 64 bit integers
   /// 'file format'
   unsigned char byteCnt;
};

std::map<std::string, RawDataset> DATASETS = {
   //   {"wiki", {"wiki_ts_200M_uint64", 8}},
   //   {"osm_cellids", {"osm_cellids_200M_uint64", 8}},
   //   {"books64", {"books_200M_uint64", 8}},
   //   {"fb", {"fb_200M_uint64", 8}},
   //   {"books32", {"books_200M_uint32", 4}}
};

template<typename T>
std::vector<std::string> keys(std::map<std::string, T> const& map) {
   std::vector<std::string> ret;
   for (auto const& element : map) {
      ret.push_back(element.first);
   }
   return ret;
}
std::vector<std::string> DATASET_NAMES = keys(DATASETS);

std::optional<std::string> option(int argc, char* argv[], const std::string& option) {
   for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      auto start = arg.find(option);
      if (0 == start) {
         return arg.substr(option.length(), arg.length() - option.length());
      }
   }
   return {};
}

Args parse(int argc, char* argv[]) {
   // TODO: set sane defaults
   //   Dataset dataset;
   double over_alloc = 1.0;
   int bucket_size = 1;

   // Parse over_alloc parameter
   if (auto raw_over_alloc = option(argc, argv, "-over-alloc=")) {
      over_alloc = std::stod(*raw_over_alloc);
   }

   // Parse bucket_size parameter
   if (auto raw_bucket_size = option(argc, argv, "-bucket-size=")) {
      bucket_size = std::stoi(*raw_bucket_size);
   }

   // Parse dataset parameter
   if (auto raw_dataset = option(argc, argv, "-dataset=")) {
      auto it = DATASETS.find(*raw_dataset);
      if (it != DATASETS.end()) {
         auto dataset = it->second;
         std::cout << "Dataset file: " << it->second.file << std::endl;
         // TODO: load dataset etc
      } else {
         std::cerr << "Unknown dataset: " << *raw_dataset << std::endl;
         exit(ExitCode::UNKNOWN_DATASET);
      }
   } else {
      std::cerr << "Please specify a dataset using \"-dataset=<DATASET>\"" << std::endl;
      exit(ExitCode::MISSING_ARGUMENT);
   }

   return {.dataset = {}, .over_alloc = over_alloc, .bucket_size = bucket_size};
}
