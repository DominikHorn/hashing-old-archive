#pragma once

#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct CSV {
   CSV(const std::string outfile_path, const std::vector<std::string> columns) : columns(columns) {
      outfile.open(outfile_path);

      std::unique_lock<std::mutex> lock(mutex);
      // Write header immediately
      for (size_t i = 0; i < columns.size(); i++) {
         outfile << columns[i];
         if (i + 1 < columns.size()) {
            outfile << ",";
         }
      }
      outfile << std::endl;
   };

   /**
    * Writes a row of csv
    * @param data maps field name/key to value. If no value is present for a key, this function will write an empty column
    */
   void write(const std::map<std::string, std::string>& data) {
      std::unique_lock<std::mutex> lock(mutex);
      for (size_t i = 0; i < columns.size(); i++) {
         const auto& field = columns[i];

         auto it = data.find(field);
         if (it != data.end()) {
            const std::string& str = it->second;
            outfile << str;
         }

         if (i + 1 < columns.size()) {
            outfile << ",";
         }
      }
      outfile << std::endl;
   }

  private:
   const std::vector<std::string> columns;
   std::ofstream outfile;
   std::mutex mutex;
};