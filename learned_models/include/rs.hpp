#pragma once

#include "rs/builder.h"
#include "rs/radix_spline.h"

#include <iostream>
#include <string>
#include <vector>

namespace rs {
   template<class Data,
            const size_t NumRadixBits = 18,
            const size_t MaxError = 32,
            const size_t MaxModels = std::numeric_limits<size_t>::max()>
   struct RadixSplineHash {
      template<class RandomIt>
      RadixSplineHash(const RandomIt& sample_begin, const RandomIt& sample_end, const size_t full_size)
         // output \in [0, sample_size] -> multiply with (full_size / sample_size)
         : out_scale_fac(static_cast<double>(full_size) /
                         static_cast<double>(std::distance(sample_begin, sample_end))) {
         const Data min = *sample_begin;
         const Data max = *(sample_end - 1);
         rs::Builder<Data> rsb(min, max, NumRadixBits, MaxError);
         for (auto it = sample_begin; it < sample_end; it++)
            rsb.AddKey(*it);

         spline = rsb.Finalize();
      }

      static std::string name() {
         return "radix_spline_err" + std::to_string(MaxError) + "_rbits" + std::to_string(NumRadixBits);
      }

      template<class Result = size_t>
      forceinline Result operator()(const Data& key) const {
         return static_cast<Result>(spline.GetEstimatedPosition(key) * out_scale_fac);
      }

     private:
      const double out_scale_fac;
      rs::RadixSpline<Data> spline;
   };
} // namespace rs