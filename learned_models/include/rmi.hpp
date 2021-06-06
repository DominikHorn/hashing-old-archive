#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include <convenience.hpp>

namespace rmi {
   template<class X, class Y>
   struct DatapointImpl {
      X x;
      Y y;
   };

   template<class Key, class Precision>
   struct LinearImpl {
     protected:
      Precision slope = 0, intercept = 0;

     private:
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
      explicit LinearImpl(Precision slope = 0, Precision intercept = 0) : slope(slope), intercept(intercept) {}

      /**
       * Performs trivial linear regression on the datapoints (i.e., computing max->min spline)
       *
       * @param datapoints *sorted* datapoints to 'train' on
       * @param output_range outputs will be in range [0, output_range] 
       */
      explicit LinearImpl(const std::vector<Datapoint>& datapoints)
         : slope(compute_slope(datapoints.front(), datapoints.back())),
           intercept(compute_intercept(datapoints.front(), datapoints.back())) {}

      /**
       * Extrapolates an index for the given key to the range [0, max_value]
       *
       * @param k key value to extrapolate for
       * @param max_value output indices are \in [0, max_value]. Defaults to std::numeric_limits<Precision>::max()
       */
      forceinline size_t operator()(const Key& k,
                                    const Precision& max_value = std::numeric_limits<Precision>::max()) const {
         // (slope * k + intercept) \in [0, 1] by construction
         const auto pred = static_cast<size_t>((max_value + 1) * (slope * k + intercept));

         // clamp (just in case). TODO(dominik): remove this additional clamp
         if (unlikely(pred > max_value))
            return max_value;
         return pred;
      }
   };

   template<class Key,
            size_t SecondLevelModelCount,
            class RootModel = LinearImpl<Key, double>,
            class SecondLevelModel = LinearImpl<Key, double>>
   struct RMIHash {
     private:
      using Precision = double;
      using Datapoint = DatapointImpl<Key, Precision>;
      using Linear = LinearImpl<Key, Precision>;

      /// Root model
      const RootModel root_model;

      /// Second level models
      std::array<Linear, SecondLevelModelCount> second_level_models;

      /// prediction range for each second level model & full output range ([0, full_size])
      const size_t second_level_range, full_size;

      /// due to rounding, some slots can potentially be empty at the end of the hashtable.
      const size_t missing_slots;

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
         : root_model(Linear({{.x = *sample_begin, .y = 0}, {.x = *sample_end, .y = 1}})),
           second_level_range(full_size / SecondLevelModelCount), full_size(full_size),
           missing_slots(std::max(static_cast<size_t>(0), full_size - SecondLevelModelCount * second_level_range)) {
         // Assign each sample point into a training bucket according to root model
         std::vector<std::vector<Datapoint>> training_buckets(SecondLevelModelCount);
         const auto sample_size = std::distance(sample_begin, sample_end);

         for (auto [it, i] = std::tuple{sample_begin, 0}; it < sample_end; it++, i++) {
            // Predict second level model using root model and put
            // sample datapoint into corresponding training bucket
            const auto key = *it;
            const auto second_level_index = root_model(key, SecondLevelModelCount - 1);
            auto& bucket = training_buckets[second_level_index];

            // The following works because the previous training bucket has to be completed,
            // because the sample is sorted:
            // Each training bucket's min is the previous training bucket's max (except for first bucket)
            if (bucket.empty() && second_level_index == 0)
               bucket.push_back(training_buckets[second_level_index - 1].back());

            // Add datapoint at the end of the bucket
            bucket.push_back({.x = key, .y = static_cast<Precision>(i) / static_cast<Precision>(sample_size)});
         }

         // Edge case: First model does not have enough training data -> add artificial datapoints
         while (training_buckets[0].size() < 2)
            training_buckets[0].insert(training_buckets[0].begin(), Datapoint(0, 0));

         // Train each second level model on its respective bucket
         for (size_t model_idx = 0; model_idx < SecondLevelModelCount; model_idx++) {
            auto& training_bucket = training_buckets[model_idx];

            // Propagate datapoints from previous training bucket if necessary
            assert(training_bucket.size() >= 1);
            if (training_bucket.size() < 2)
               training_bucket.insert(training_bucket.begin(), training_buckets[model_idx - 1].back());
            assert(training_bucket.size() >= 2);

            // Train model on training bucket & add it
            second_level_models[model_idx] = SecondLevelModel(training_bucket);
         }
      }

      static std::string name() {
         return "rmi_hash_" + std::to_string(SecondLevelModelCount);
      }

      size_t model_count() {
         return 1 + SecondLevelModelCount;
      }

      /**
       * Compute hash value for key
       *
       * @tparam Result result data type. Defaults to size_t
       * @param key
       */
      template<class Result = size_t>
      forceinline Result operator()(const Key& key) const {
         const auto second_level_index = root_model(key, SecondLevelModelCount - 1);

         // Output for this model is \in [output_min, output_max]
         auto output_min = second_level_index * second_level_range;
         auto output_max = second_level_range;

         // Necessary to ensure [0, full_range] output (otherwise a few slots might be unused)
         if (second_level_index == SecondLevelModelCount - 1)
            output_max += missing_slots;

         return output_min + second_level_models[second_level_index](key, output_max);
      }
   };
} // namespace rmi
