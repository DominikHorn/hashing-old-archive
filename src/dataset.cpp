#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "convenience.hpp"
#include "dataset.hpp"

/**
 * Helper for extracting all keys from a map
 * @tparam T
 */
template<typename T>
std::vector<std::string> keys(std::map<std::string, T> const& map) {
   std::vector<std::string> ret;
   for (auto const& element : map) {
      ret.push_back(element.first);
   }
   return ret;
}

/**
 * Helper to extract an 8 byte number encoded in little endian from a byte vector
 */
uint64_t forceinline read_little_endian_8(const std::vector<unsigned char>& buffer, uint64_t offset) {
   return (uint64_t) buffer[offset + 0] | ((uint64_t) buffer[offset + 1] << 8) |
      ((uint64_t) buffer[offset + 2] << (2 * 8)) | ((uint64_t) buffer[offset + 3] << (3 * 8)) |
      ((uint64_t) buffer[offset + 4] << (4 * 8)) | ((uint64_t) buffer[offset + 5] << (5 * 8)) |
      ((uint64_t) buffer[offset + 6] << (6 * 8)) | ((uint64_t) buffer[offset + 7] << (7 * 8));
}

/**
 * Helper to extract a 4 byte number encoded in little endian from a byte vector
 */
uint64_t forceinline read_little_endian_4(const std::vector<unsigned char>& buffer, uint64_t offset) {
   return buffer[offset + 0] | (buffer[offset + 1] << 8) | (buffer[offset + 2] << (2 * 8)) |
      (buffer[offset + 3] << (3 * 8));
}

std::map<std::string, Dataset> DATASETS = {
   //
   {"wiki", {.filename = "wiki_ts_200M_uint64", .bytesPerValue = BytesPerValue::_64bit}},
   {"osm_cellids", {.filename = "osm_cellids_200M_uint64", .bytesPerValue = BytesPerValue::_64bit}},
   {"books64", {.filename = "books_200M_uint64", .bytesPerValue = BytesPerValue::_64bit}},
   {"fb", {.filename = "fb_200M_uint64", .bytesPerValue = BytesPerValue::_64bit}},
   {"books32", {.filename = "books_200M_uint32", .bytesPerValue = BytesPerValue::_32bit}},
   {"dense32", {.filename = "dense_32", .bytesPerValue = BytesPerValue::_32bit}},
   {"dense64", {.filename = "dense_64", .bytesPerValue = BytesPerValue::_64bit}},
   {"gapped5", {.filename = "gapped5_64", .bytesPerValue = BytesPerValue::_64bit}},
   {"debug", {.filename = "debug_64", .bytesPerValue = BytesPerValue::_64bit}}
   //
};
std::vector<std::string> DATASET_NAMES = keys(DATASETS);

std::vector<uint64_t> Dataset::load(const std::string& path) const {
   const auto filepath = path + this->filename + ".ds";

   // Read file into memory from disk. Directly map file for more performance
   std::ifstream input(filepath, std::ios::binary | std::ios::ate);
   std::streamsize size = input.tellg();
   input.seekg(0, std::ios::beg);
   if (!input.is_open()) {
      throw "Dataset file at path '" + filepath + "' does not exist";
   }
   std::vector<unsigned char> buffer(size);
   if (!input.read((char*) buffer.data(), size)) {
      throw "Failed to read dataset at path '" + filepath + "'";
   }

   // Parse file
   uint64_t num_elements = read_little_endian_8(buffer, 0);
   std::vector<uint64_t> dataset(num_elements, 0);
   if (this->bytesPerValue == BytesPerValue::_64bit)
      for (uint64_t i = 0; i < num_elements; i++) {
         // 8 byte header, 8 bytes per entry
         uint64_t offset = i * 8 + 8;
         dataset[i] = read_little_endian_8(buffer, offset);
      }
   else if (this->bytesPerValue == BytesPerValue::_32bit)
      for (uint64_t i = 0; i < num_elements; i++) {
         // 8 byte header, 4 bytes per entry
         uint64_t offset = i * 4 + 8;
         dataset[i] = read_little_endian_4(buffer, offset);
      }
   else {
      throw "Unimplemented amount of bytes per value in dataset: " + std::to_string(this->bytesPerValue);
   }

   std::cout << "Read " << num_elements << " entries from dataset file" << filepath << std::endl;

   // Sort and deduplicate dataset
   std::sort(dataset.begin(), dataset.end());
   dataset.erase(std::unique(dataset.begin(), dataset.end()), dataset.end());

   // TODO: what did the log2(dataset[dataset.size() - 1]) = log2(max(dataset)) respective - 2 achieve?
   std::cout << "Sorted and deduplicated dataset. " << dataset.size() << " entries remaining" << std::endl;

   return dataset;
}
