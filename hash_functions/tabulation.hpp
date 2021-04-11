#pragma once

#include <fstream>
#include <iostream>

#include "../convenience/convenience.hpp"
#include "types.hpp"

/**
 *
 * ----------------------------
 *       Tabulation Hash
 * ----------------------------
 */

struct TabulationHash {
   /**
    * Naive tabulation hashing, i.e., no manual vectorization
    *
    * @tparam T
    * @param value the value to hash
    * @param table tabulation lookup table
    * @param seed seed value, defaults to 0
    * @return
    */
   template<typename T>
   static forceinline unroll_loops T naive_hash(const T& value, const T (&table)[sizeof(T)][0xFF], const T& seed = 0) {
      T out = seed;
      for (auto i = 0; i < sizeof(T); i++) {
         out ^= table[i][(uint8_t) (value >> 8 * i)];
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
   static forceinline unroll_loops void gen_table(T table[COLUMNS][0xFF], unsigned int seed = 1) {
      // TODO: consider better (but unseeded) random function:
      //      std::ifstream file("/dev/random", std::ios::in | std::ios::binary | std::ios::ate);
      //      if (file.is_open()) {
      //         file.seekg(0, std::ios::beg);
      //         file.read((char*) table, sizeof(T) * sizeof(T) * 0xFF);
      //      }

      srand(seed);

      const auto start = (uint8_t*) &table[0][0];
      for (auto c = 0; c < COLUMNS; c++) {
         for (auto r = 0; r < 0xFF; r++) {
            table[c][r] = 0;
            for (auto i = 0; i < COLUMNS; i++) {
               table[c][r] |= ((T) rand() % 256) << 8 * i;
            }
         }
      }
   }

   template<typename T, size_t COLUMNS = sizeof(T), size_t ROWS = 0xFF>
   static void print_table(const T table[COLUMNS][ROWS]) {
      std::cout << "addr\t";
      for (auto c = 0; c < COLUMNS; c++) {
         std::cout << "col " << c << "\t";
      }
      std::cout << std::endl;

      for (auto r = 0; r < ROWS; r++) {
         std::cout << std::hex << r << "\t\t";
         for (auto c = 0; c < COLUMNS; c++) {
            std::cout << std::hex << table[c][r] << "\t\t";
         }
         std::cout << std::endl;
      }
   }
};