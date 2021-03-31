#ifndef CONVENIENCE_H
#define CONVENIENCE_H

#ifdef __GNUC__
#define forceinline __attribute__((always_inline))
#define packed __attribute__((packed))
#endif

#endif