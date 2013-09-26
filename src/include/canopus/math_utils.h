#ifndef _CANOPUS_MATH_UTILS_H
#define _CANOPUS_MATH_UTILS_H

#include <stdint.h>

/* i ^ n */
int ipow(uint32_t n, uint32_t i);

#if defined(__GNUC__)
# if defined(__TMS470__) || defined(__TI_ARM__) /* --gcc enabled */
#  define BITCOUNT_MISSING
# endif
#else
# define BITCOUNT_MISSING
#endif

#ifdef BITCOUNT_MISSING
int bitcount(unsigned int x);
#else
# define bitcount __builtin_popcount
#endif

#endif
