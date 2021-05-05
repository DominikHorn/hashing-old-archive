#pragma once

#include <convenience.hpp>

struct LinearProbingFunc {
  public:
   static std::string name() {
      return "linear";
   }

   forceinline size_t operator()(const size_t& index, const size_t& probing_step, const size_t& directory_size) const {
      const auto next = index + probing_step;
      // TODO: use fast modulo operation to make this truely competitive
      return next >= directory_size ? next % directory_size : next;
   }
};

struct QuadraticProbingFunc {
  public:
   static std::string name() {
      return "quadratic";
   }

   forceinline size_t operator()(const size_t& index, const size_t& probing_step, const size_t& directory_size) const {
      const auto next = index + probing_step * probing_step;
      // TODO: use fast modulo operation to make this truely competitive
      return next >= directory_size ? next % directory_size : next;
   }
};
