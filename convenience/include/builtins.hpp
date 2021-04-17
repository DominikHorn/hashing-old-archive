#pragma once

#ifdef __GNUC__
   /**
 * These optimizing compiler hints were taken from
 * https://github.com/toschmidt/cpp-utility/blob/master/src/cpp-utility/compiler/CompilerHints.hpp#L8
 * (commit: 46a5c6b)
 *
 * We only want to perform them in release builds
 */
   #ifdef NDEBUG
      #define forceinline inline __attribute__((always_inline))
      #define unroll_loops __attribute__((optimize("unroll-loops")))
      #define likely(expr) __builtin_expect((expr), 1)
      #define unlikely(expr) __builtin_expect((expr), 0)
   #else
      #define forceinline
      #define unroll_loops
      #define likely(expr) expr
      #define unlikely(expr) expr
   #endif

   #define neverinline __attribute__((noinline))
   #define packed __attribute__((packed))
   #define prefetch(address, mode, locality) __builtin_prefetch(address, mode, locality)
#else
// TODO: we will need clang support, therefore verify that the above primitives all work with clang
   #error Your compiler is not supported
#endif
