#pragma once

#ifdef __GNUC__
   #define packed __attribute__((packed))

   /**
    * The following compiler hints were taken from
    * https://github.com/toschmidt/cpp-utility/blob/master/src/cpp-utility/compiler/CompilerHints.hpp#L8
    * (commit: 46a5c6b)
    */
   #define forceinline inline __attribute__((always_inline))
   #define unroll_loops __attribute__((optimize("unroll-loops")))
   #define likely(expr) __builtin_expect((expr), 1)
   #define unlikely(expr) __builtin_expect((expr), 0)
#endif