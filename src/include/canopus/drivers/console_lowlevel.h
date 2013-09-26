#ifndef _CANOPUS_DRIVERS_CONSOLE_LOWLEVEL_H
#define _CANOPUS_DRIVERS_CONSOLE_LOWLEVEL_H

#include <canopus/types.h>

/*
 * binary
 */

void lowlevel_console_putbuf(const void *buf, size_t count);

void lowlevel_console_getbuf(void *buf, size_t count);

/*
 * user interaction
 */

#ifdef DEBUG_CONSOLE_ENABLED

# ifdef __arm__THIS_IS_NOT_WORKING_YET
#  define ATTR_NAKED __attribute__((naked))
# else
#  define ATTR_NAKED
# endif

typedef enum {
    HEXDUMP_BASIC       = 1<<0,
    HEXDUMP_ADDRESS     = 1<<1,
    HEXDUMP_GROUP4_32   = 1<<2
} hexdump_flag_e;

void lowlevel_console_putchar(char c) ATTR_NAKED;

char lowlevel_console_getchar(void) ATTR_NAKED;

void lowlevel_console_putnewline(void) ATTR_NAKED;

void lowlevel_console_putstring(const char *string) ATTR_NAKED;

void lowlevel_console_puthex8(uint8_t val8) ATTR_NAKED;

void lowlevel_console_puthex32(uint32_t val32) ATTR_NAKED;

void lowlevel_console_putstrhex32(const char *string, uint32_t val32) ATTR_NAKED;

void lowlevel_console_hexdump(const void *ptr, size_t size, const char *desc) ATTR_NAKED;

void lowlevel_console_hexdump_ex(const void *ptr, size_t size, const char *desc, hexdump_flag_e flags);

#else

#define lowlevel_console_putchar(c)
#define lowlevel_console_getchar() 0
#define lowlevel_console_putnewline()
#define lowlevel_console_putstring(s) (void)(s)
#define lowlevel_console_puthex32(v) (void)(v)
#define lowlevel_console_putstrhex32(s, v) (void)(s); (void)(v)
#define lowlevel_console_hexdump(p, s, d) (void)(p); (void)(d)
#define lowlevel_console_hexdump_ex(p, s, d, f) (void)(p); (void)(d)

#endif /* DEBUG_CONSOLE_ENABLED */

#endif
