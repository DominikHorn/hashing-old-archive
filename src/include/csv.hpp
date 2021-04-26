#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

struct CSV {
   CSV(const std::string outfile_path, const std::vector<std::string> columns) : columns(columns) {
      bool created_file = !std::filesystem::exists(outfile_path);
      outstream.open(outfile_path, std::ios::app | std::ios::out);
      if (!outstream.is_open()) {
         throw std::runtime_error("Failed to open output csv " + outfile_path);
      }

      if (created_file) {
         // Write header immediately if file is new
         std::unique_lock<std::mutex> lock(out_mutex);
         outstream << header() << std::endl;
      } else {
         // Load existing file contents
         std::ifstream infile(outfile_path);
         if (!infile.is_open()) {
            throw std::runtime_error("Outfile " + outfile_path +
                                     " exists but could not be opened for parsing existing results");
         }

         std::string h_line;
         if (!std::getline(infile, h_line)) {
            // file is empty
            std::unique_lock<std::mutex> lock(out_mutex);
            outstream << header() << std::endl;
            return;
         }

         // Check that header matches
         if (header().compare(trim(h_line)) != 0) {
            throw std::runtime_error("Expected csv header \n\t'" + header() + "' but existing file has \n\t'" + h_line +
                                     "'");
         }

         // Index over existing lines
         std::string line;
         while (std::getline(infile, line)) {
            std::istringstream iss(line);
            char ch = ',';

            std::map<std::string, std::string> fields;
            std::string field;
            for (const auto& column : columns) {
               field = "";
               do {
                  if (iss.eof())
                     break;
                  else
                     iss >> std::noskipws >> ch;

                  if (ch != ',')
                     field += ch;
               } while (ch != ',');
               fields.emplace(column, field);
            }

            existing_entries.emplace_back(fields);
         }
      }
   };

   /**
    * Writes a row of csv
    * @param data maps field name/key to value. If no value is present for a key, this function will write an empty column
    */
   void write(const std::map<std::string, std::string>& data) {
      std::unique_lock<std::mutex> lock(out_mutex);
      for (size_t i = 0; i < columns.size(); i++) {
         const auto& field = columns[i];

         auto it = data.find(field);
         if (it != data.end()) {
            const std::string& str = it->second;
            outstream << str;
         }

         if (i + 1 < columns.size()) {
            outstream << ",";
         }
      }
      outstream << std::endl;
   }

   /**
    * Checks if a row with matching entries for the specified columns exists
    * @param signature
    * @return
    */
   bool exists(const std::map<std::string, std::string>& signature) {
      return std::find_if(existing_entries.begin(), existing_entries.end(), [&](const auto entry) {
                for (const auto& it : signature) {
                   const auto elem = entry.find(it.first);
                   if (elem == entry.end() || elem->second != it.second)
                      return false;
                }
                return true;
             }) != existing_entries.end();
   }

  private:
   const std::vector<std::string> columns;
   std::vector<std::map<std::string, std::string>> existing_entries;
   std::ofstream outstream;
   std::mutex out_mutex;

   inline std::string header() {
      std::string header = "";
      for (size_t i = 0; i < columns.size(); i++) {
         header += columns[i];
         if (i + 1 < columns.size()) {
            header += ",";
         }
      }
      return header;
   }

   static inline std::string trim(std::string s) {
      s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
      s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
      return s;
   }
};