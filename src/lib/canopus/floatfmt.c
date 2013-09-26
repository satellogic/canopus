#include <canopus/floatfmt.h>
#include <canopus/stringformat.h>

#ifdef USE_LIBM
# include <math.h>
# define TRUNC(f) trunc(f)
#else
# define TRUNC(f) (int)(f)
#endif

#define FLOATFMT_MAXDIGITS 4

char *
ftoad(float f, char *buf, size_t bufsize, int digits)
{
    const static uint16_t ipow10[FLOATFMT_MAXDIGITS + 1] = {
        1, 10, 100, 1000, 10000
    };
    int d1;

    d1 = TRUNC(f);    /* integer part */
    if (digits <= 0) {
        snprintf(buf, bufsize, "%d", d1);
    } else {
        float f2 = f - d1;  /* fractional part */
        int d2;
        char fmt[] = "%d.%0xd";

        if (f < 0) f2 *= -1;
        if (digits >= FLOATFMT_MAXDIGITS) digits = FLOATFMT_MAXDIGITS;
        d2 = TRUNC(f2 * ipow10[digits]);
        fmt[5] = '0' + digits;
        snprintf(buf, bufsize, fmt, d1, d2);
    }

    return buf;
}

char *
ftoa(float f, char *buf, size_t bufsize)
{
    return ftoad(f, buf, bufsize, FLOATFMT_MAXDIGITS);
}

