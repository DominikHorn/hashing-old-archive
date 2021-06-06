#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <convenience.hpp>

namespace rmi {
   template<class X, class Y>
   struct DatapointImpl {
      const X x;
      const Y y;
   };

   template<class Key, class Precision>
   struct LinearImpl {
     private:
      Precision slope = 0, intercept = 0;
      using Datapoint = DatapointImpl<Key, Precision>;

      static forceinline Precision compute_slope(const Datapoint& min, const Datapoint& max) {
         // slope = delta(y)/delta(x)
         return ((max.y - min.y) / (max.x - min.x));
      }

      static forceinline Precision compute_intercept(const Datapoint& min, const Datapoint& max) {
         // f(min.x) = min.y <=> slope * min.x + intercept = min.y <=> intercept = min.y - slope * min_key
         return (min.y - compute_slope(min, max) * min.x);
      }

     public:
      LinearImpl() = default;

      /**
       * Performs trivial linear regression on the datapoints (i.e., computing max->min spline)
       *
       * @param datapoints *sorted* datapoints to 'train' on
       * @param output_range outputs will be in range [0, output_range]. Defaults to 1
       */
      explicit LinearImpl(const std::vector<Datapoint>& datapoints, const size_t output_range = 1)
         // rescale output to [0, output_range]: (ax + b) * c = (c*ax + c+b)
         : slope(output_range * compute_slope(datapoints.front(), datapoints.back())),
           intercept(output_range * compute_intercept(datapoints.front(), datapoints.back())) {}

      template<class R = size_t>
      forceinline R operator()(const Key& k) const {
         return static_cast<R>(slope * k + intercept);
      }
   };

   template<class Key, size_t SecondLevelModels, class RootModel = LinearImpl<Key, double>>
   struct RMIHash {
     private:
      using Precision = double;
      using Datapoint = DatapointImpl<Key, Precision>;
      using Linear = LinearImpl<Key, Precision>;

      RootModel root_model;
      std::array<Linear, SecondLevelModels> second_level_models;

     public:
      /**
       * Builds rmi on an already sorted (!) sample
       * @tparam RandomIt
       * @param sample_begin
       * @param sample_end
       * @param full_size operator() will extrapolate to [0, full_size]
       * @param models_per_layer
       * @param sample_size
       */
      template<class RandomIt>
      RMIHash(const RandomIt& sample_begin, const RandomIt& sample_end, const size_t full_size)
         : root_model(Linear({{.x = *sample_begin, .y = 0}, {.x = *sample_end, .y = 1}}, SecondLevelModels)) {
         // Assign each sample point into a training bucket according to root model
         std::vector<std::vector<Datapoint>> training_buckets(SecondLevelModels);
         const auto sample_size = std::distance(sample_begin, sample_end);

         for (auto [it, i] = std::tuple{sample_begin, 0}; it < sample_end; it++, i++) {
            // Predict second level model using root model and put sample datapoint into corresponding training bucket
            const auto key = *it;
            unsigned int second_level_index = std::clamp(root_model(key), static_cast<size_t>(0), SecondLevelModels);
            auto& bucket = training_buckets[second_level_index];
            bucket.push_back({.x = key, .y = static_cast<Precision>(i) / static_cast<Precision>(sample_size)});
         }

         // Train each second level model on its respective bucket
#warning "TODO: RMIHash second level model build unimplemented"
      }

      static std::string name() {
         return "rmi_hash_" + std::to_string(SecondLevelModels);
      }

      template<class Result = size_t>
      forceinline Result operator()(const Key& key) const {
#warning "TODO: RMIHash inference unimplemented"
         return 0;
      }
   };
} // namespace rmi
