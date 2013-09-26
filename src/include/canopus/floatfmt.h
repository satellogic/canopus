#ifndef _CANOPUS_FLOATFMT_H
#define _CANOPUS_FLOATFMT_H

#include <stdint.h>
#include <canopus/types.h>

#define FLOATFMT_BUFSZ_MAX 18

/* default float to ascii routine */
char *ftoa(float f, char *buf, size_t bufsize);

/* up to 'digits' precision */
char *ftoad(float f, char *buf, size_t bufsize, int digits);

#endif
