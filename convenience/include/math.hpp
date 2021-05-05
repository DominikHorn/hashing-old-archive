#pragma once

#include "builtins.hpp"

template<class T>
static constexpr forceinline T varmax(const T& t1, const T& t2) {
   return std::max(t1, t2);
}

template<class T, class... Args>
static constexpr forceinline T varmax(const T& t1, const T& t2, Args... args) {
   return varmax(varmax(t1, t2), args...);
}