#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include <convenience.hpp>

template<class T, const T seed = 0, size_t COLUMNS = sizeof(T), size_t ROWS = 0xFF>
struct _TabulationHashImplementation {
   static std::string name() {
      return "tabulation_" + std::to_string(COLUMNS) + "x" + std::to_string(ROWS) + "_" + std::to_string(sizeof(T) * 8);
   }

   /**
       * Initializes a tabulation hash tables with random data, depending on seed
       * @tparam T hash output type, e.g., HASH_64
       * @param table, must have 255 rows
       * @param seed defaults to 1
       */
   _TabulationHashImplementation(unsigned int rand_seed = 0x238EF8E3LU) {
      // TODO: consider better (but unseeded) random function:
      //      std::ifstream file("/dev/random", std::ios::in | std::ios::binary | std::ios::ate);
      //      if (file.is_open()) {
      //         file.seekg(0, std::ios::beg);
      //         file.read((char*) table, sizeof(T) * sizeof(T) * 0xFF);
      //      }

      const auto gen_column = [](std::array<T, ROWS>& column) {
         for (auto& r : column) {
            r = 0;
            for (size_t i = 0; i < sizeof(T); i++) {
               r |= static_cast<T>(rand() % 256) << 8 * i;
            }
         }
      };

      srand(rand_seed);
      for (size_t c = 0; c < COLUMNS; c++) {
         gen_column(table[c]);
      }
   }

   constexpr forceinline T operator()(const T& key) const {
      T out = seed;

      for (size_t i = 0; i < sizeof(T); i++) {
         out ^= table[i % COLUMNS][static_cast<uint8_t>(key >> (8 * i)) % ROWS];
      }
      return out;
   }

  private:
   std::array<std::array<T, ROWS>, COLUMNS> table;

   void print_table() {
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

/**
 * Small tabulation hash, i.e., single column
 */
template<class T, const T seed = 0>
using SmallTabulationHash = _TabulationHashImplementation<T, seed, 1, 0xFF>;

/**
 * Medium tabulation hash, i.e., four columns
 */
template<class T, const T seed = 0>
using MediumTabulationHash = _TabulationHashImplementation<T, seed, 4, 0xFF>;

/**
 * Large tabulation hash, i.e., eight columns
 */
template<class T, const T seed = 0>
using LargeTabulationHash = _TabulationHashImplementation<T, seed, 8, 0xFF>;
