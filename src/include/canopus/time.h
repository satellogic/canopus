#ifndef _CANOPUS_TIME_H_
#define _CANOPUS_TIME_H_

#include <stdint.h>
#include <canopus/types.h>
#ifdef _POSIX_C_SOURCE
# include <time.h>
#endif

/*
 * timespec is only defined in POSIX.1b
 * thus we can't include /usr/include/time.h
 * defining an equivalent type here
 */
#if defined(TIMESPEC_TYPE_MISSING) && !defined(__timespec_defined)
# define __timespec_defined 1 /* fix for linux <time.h> */
struct timespec {
	uint32_t tv_sec;
	uint32_t tv_nsec;
};
#endif

typedef struct timespec timespec_t;

typedef uint32_t timeout_t;

/**
 * @retval RV_SUCCESS, RV_NOSPACE, RV_ERROR
 */
retval_t timespec_str(char *dest, size_t size, const timespec_t *timespec);

retval_t timespec_copy(timespec_t *dest, const timespec_t *src);

#endif
