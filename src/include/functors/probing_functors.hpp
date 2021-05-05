#pragma once

#include <convenience.hpp>

struct LinearProbingFunc {
  public:
   static std::string name() {
      return "linear";
   }

   forceinline size_t operator()(const size_t& previous, const size_t& max) const {
      const auto next = previous + 1;
      if (unlikely(next >= max))
         return 0;
      return next;
   }
};