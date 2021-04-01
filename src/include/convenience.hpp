#pragma once

#ifdef __GNUC__
   #define forceinline __attribute__((always_inline))
   #define packed __attribute__((packed))
#endif