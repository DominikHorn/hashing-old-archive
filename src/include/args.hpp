#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * Parsed command line args of the application
 */
struct Args {
  public:
   const double over_alloc;
   const int bucket_size;
   const std::vector<uint64_t> dataset;

   /**
    * Parses the given set of command line args
    */
   static Args parse(const int argc, const char* argv[]);

  private:
   const std::string datapath;

   static std::optional<std::string> option(const int argc, const char* argv[], const std::string& option);

   Args(const double over_alloc, const int bucket_size, const std::vector<uint64_t> dataset, const std::string datapath)
      : over_alloc(over_alloc), bucket_size(bucket_size), dataset(dataset), datapath(datapath){};
};