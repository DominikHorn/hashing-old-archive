#pragma once

#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <convenience.hpp>

struct Dataset {
  public:
   /// file name of the dataset
   std::string filepath;

   /// Bytes per value, i.e., 4 for 32-bit integers, 8 for 64 bit integers
   size_t bytesPerValue;

   std::string name() const {
      return filepath.substr(filepath.find_last_of("/\\") + 1);
   }

   /**
    * Loads the datasets values into memory
    * @return a sorted and deduplicated list of all members of the dataset
    */
   std::vector<uint64_t> load(std::mutex& iomutex) const {
#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "Loading dataset " << filepath << " ... " << std::flush;
      }
#endif

      // Read file into memory from disk. Directly map file for more performance
      std::ifstream input(filepath, std::ios::binary | std::ios::ate);
      std::streamsize size = input.tellg();
      input.seekg(0, std::ios::beg);
      if (!input.is_open()) {
         throw std::runtime_error("Dataset file at data_folder_path '" + filepath + "' does not exist");
      }

      const auto max_num_elements = (size - sizeof(uint64_t)) / bytesPerValue;
      std::vector<uint64_t> dataset(max_num_elements, 0);
      {
         std::vector<unsigned char> buffer(size);
         if (!input.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Failed to read dataset at data_folder_path '" + filepath + "'");
         }

         // Parse file
         uint64_t num_elements = read_little_endian_8(buffer, 0);
         assert(num_elements <= max_num_elements);
         if (bytesPerValue == 8)
            for (uint64_t i = 0; i < num_elements; i++) {
               // 8 byte header, 8 bytes per entry
               uint64_t offset = i * 8 + 8;
               dataset[i] = read_little_endian_8(buffer, offset);
            }
         else if (bytesPerValue == 4)
            for (uint64_t i = 0; i < num_elements; i++) {
               // 8 byte header, 4 bytes per entry
               uint64_t offset = i * 4 + 8;
               dataset[i] = read_little_endian_4(buffer, offset);
            }
         else {
            throw std::runtime_error("Unimplemented amount of bytes per value in dataset: " +
                                     std::to_string(this->bytesPerValue));
         }
      }

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "Sorting ... " << std::flush;
      }
#endif
      sort(dataset);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "Removing duplicates ... " << std::flush;
      }
#endif
      deduplicate(dataset);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "Fisher-Yates shuffling ... " << std::flush;
      }
#endif
      shuffle(dataset);

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "Shrinking ... " << std::flush;
      }
#endif
      dataset.shrink_to_fit();

#ifdef VERBOSE
      {
         std::unique_lock<std::mutex> lock(iomutex);
         std::cout << "done. " << dataset.size() << " elements loaded" << std::endl;
      }
#endif

      return dataset;
   }

  private:
   /**
    * Sorts a dataset using std::sort
    * @param dataset
    */
   static forceinline void sort(std::vector<uint64_t>& dataset) {
      std::sort(dataset.begin(), dataset.end());
   }

   /**
    * Deduplicates the dataset. NOTE: data must be sorted for this to work
    * @param dataset
    */
   static forceinline void deduplicate(std::vector<uint64_t>& dataset) {
      dataset.erase(std::unique(dataset.begin(), dataset.end()), dataset.end());
   }

   /**
    * Shuffles the given dataset
    * @param dataset
    * @param seed
    */
   static forceinline void shuffle(std::vector<uint64_t>& dataset, const uint64_t seed = std::random_device()()) {
      if (dataset.empty())
         return;

      std::default_random_engine gen(seed);
      std::uniform_int_distribution<uint64_t> dist(0);

      // Fisher-Yates shuffle
      for (size_t i = dataset.size() - 1; i > 0; i--) {
         std::swap(dataset[i], dataset[dist(gen) % i]);
      }
   }

   /**
    * Helper to extract an 8 byte number encoded in little endian from a byte vector
    */
   static forceinline uint64_t read_little_endian_8(const std::vector<unsigned char>& buffer, uint64_t offset) {
      return static_cast<uint64_t>(buffer[offset + 0]) | (static_cast<uint64_t>(buffer[offset + 1]) << 8) |
         (static_cast<uint64_t>(buffer[offset + 2]) << (2 * 8)) |
         (static_cast<uint64_t>(buffer[offset + 3]) << (3 * 8)) |
         (static_cast<uint64_t>(buffer[offset + 4]) << (4 * 8)) |
         (static_cast<uint64_t>(buffer[offset + 5]) << (5 * 8)) |
         (static_cast<uint64_t>(buffer[offset + 6]) << (6 * 8)) |
         (static_cast<uint64_t>(buffer[offset + 7]) << (7 * 8));
   }

   /**
    * Helper to extract a 4 byte number encoded in little endian from a byte vector
    */
   static forceinline uint64_t read_little_endian_4(const std::vector<unsigned char>& buffer, uint64_t offset) {
      return buffer[offset + 0] | (buffer[offset + 1] << 8) | (buffer[offset + 2] << (2 * 8)) |
         (buffer[offset + 3] << (3 * 8));
   }
};

/**
 * Overloading of the >> operator for parsing with cxxopts
 * @param is
 * @param dt
 * @return
 */
std::istream& operator>>(std::istream& is, Dataset& ds) {
   char ch = 0;
   while (!is.eof()) {
      is >> std::noskipws >> ch;
      if (ch == ':')
         break;
      ds.filepath += ch;
   };
   if (is.eof()) {
      throw std::runtime_error("Failed to parse dataset " + ds.filepath + ": Bytes per number not specified");
   }
   is >> ds.bytesPerValue;
   return is;
}
