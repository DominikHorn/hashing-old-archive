#ifndef DATASET_H
#define DATASET_H

#include <map>
#include <string>
#include <vector>

enum BytesPerValue
{
   _32bit = 4,
   _64bit = 8
};

struct Dataset {
  public:
   /// file name of the dataset
   std::string filename;

   /// Bytes per value, i.e., 4 for 32-bit integers, 8 for 64 bit integers
   BytesPerValue bytesPerValue;

   /**
    * Loads the datasets values into memory
    * @return a sorted and deduplicated list of all members of the dataset
    */
   std::vector<uint64_t> load(const std::string& path) const;
};

/// Map containing all datasets known to the application, by respective name
extern std::map<std::string, Dataset> DATASETS;

/// Names of all datasets known to the application
extern std::vector<std::string> DATASET_NAMES;

#endif