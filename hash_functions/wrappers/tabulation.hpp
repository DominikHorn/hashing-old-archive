#pragma once

#include <fstream>
#include <iostream>

#include "convenience.hpp"
#include "types.hpp"

/**
 *
 * ----------------------------
 *       Tabulation Hash
 * ----------------------------
 */

struct TabulationHash {
   /**
    *  Tabulation hashing using a small table and no manual vectorization
    *
    * @tparam T
    * @param value the value to hash
    * @param table tabulation lookup table
    * @param seed seed value, defaults to 0
    * @return
    */
   template<typename T>
   static forceinline T small_hash(const T& value, const T (&table)[0xFF], const T& seed = 0) {
      return seed ^
         (table[static_cast<uint8_t>(value >> 8 * 0)] | table[static_cast<uint8_t>(value >> 8 * 1)] |
          table[static_cast<uint8_t>(value >> 8 * 2)] | table[static_cast<uint8_t>(value >> 8 * 3)] |
          table[static_cast<uint8_t>(value >> 8 * 4)] | table[static_cast<uint8_t>(value >> 8 * 5)] |
          table[static_cast<uint8_t>(value >> 8 * 6)] | table[static_cast<uint8_t>(value >> 8 * 7)]);
   }

   /**
    * Tabulation hashing with a large table and no manual vectorization
    *
    * @tparam T
    * @param value the value to hash
    * @param table tabulation lookup table
    * @param seed seed value, defaults to 0
    * @return
    */
   template<typename T>
   static forceinline T large_hash(const T& value, const T (&table)[sizeof(T)][0xFF], const T& seed = 0) {
      T out = seed;

      for (size_t i = 0; i < sizeof(T); i++) {
         out ^= table[i][static_cast<uint8_t>(value >> 8 * i)];
      }
      return out;
   }

   /**
    * Initializes a tabulation hash table with random data, depending on seed
    * @tparam T hash output type, e.g., HASH_64
    * @param table, must have 255 rows
    * @param seed defaults to 1
    */
   template<typename T, size_t COLUMNS = sizeof(T)>
   static forceinline void gen_table(T (&table)[COLUMNS][0xFF], unsigned int seed = 0x238EF8E3LU) {
      // TODO: consider better (but unseeded) random function:
      //      std::ifstream file("/dev/random", std::ios::in | std::ios::binary | std::ios::ate);
      //      if (file.is_open()) {
      //         file.seekg(0, std::ios::beg);
      //         file.read((char*) table, sizeof(T) * sizeof(T) * 0xFF);
      //      }

      srand(seed);
      for (size_t c = 0; c < COLUMNS; c++) {
         gen_column(table[c]);
      }
   }

   template<typename T>
   static forceinline void gen_column(T (&column)[0xFF]) {
      for (auto& r : column) {
         r = 0;
         for (size_t i = 0; i < sizeof(T); i++) {
            r |= static_cast<T>(rand() % 256) << 8 * i;
         }
      }
   }

   template<typename T, size_t ROWS = 0xFF>
   static void print_column(const T (&column)[ROWS]) {
      std::cout << "col ";
      for (auto& r : column) {
         std::cout << std::hex << r << ", ";
      }
      std::cout << std::endl;
   }

   template<typename T, size_t COLUMNS = sizeof(T), size_t ROWS = 0xFF>
   static void print_table(const T (&table)[COLUMNS][ROWS]) {
      std::cout << "addr\t";
      for (size_t c = 0; c < COLUMNS; c++) {
         std::cout << "col " << c << "\t";
      }
      std::cout << std::endl;

      for (size_t r = 0; r < ROWS; r++) {
         std::cout << std::hex << r << "\t\t";
         for (size_t c = 0; c < COLUMNS; c++) {
            std::cout << std::hex << table[c][r] << "\t\t";
         }
         std::cout << std::endl;
      }
   }
};