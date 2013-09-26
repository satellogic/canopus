#ifndef _CANOPUS_ASSERT_H_
#define _CANOPUS_ASSERT_H_

#include <canopus/debug.h>

#ifdef NDEBUG
# define ENGINEERING_ASSERT(expr)
#else

void canopus_assert(const char *expression, const char *filename, const char *funcname, unsigned int line) __attribute__((__noreturn__));

void __assert_fail_canopus_style1(const char *expression,const char *filename, unsigned int line, const char *function);
void __assert_fail_canopus_style2(const char *filename, int line, const char *function, const char *expression) __attribute__ ((__noreturn__));
void __assert_fail_canopus_style3(int valid, const char *reason);
//void __assert_fail_canopus_style5(const char *reason);
void __assert_fail_canopus_style4(const char *reason, const char *filename, int line);

/* we define the hooks before to include <assert.h> */
#ifdef __USE_POSIX
# define __assert_fail __assert_fail_canopus_style1
#endif

#ifdef __ARM_EABI__
# define __assert_func __assert_fail_canopus_style2
#endif

#ifdef __TI_EABI_SUPPORT__
# define _assert       __assert_fail_canopus_style3
extern _CODE_ACCESS void _abort_msg(const char *reason);
#endif

#ifdef __MINGW32__
# define _assert       __assert_fail_canopus_style4
#endif

#ifdef ENGINEERING_VERSION
# warning ENGINEERING_ASSERT ENABLED!
# define ENGINEERING_ASSERT(expr) assert(expr)
#else
# define ENGINEERING_ASSERT(expr)
#endif

#endif /* NDEBUG */

/* warning: <assert.h> included at the end on purpose */
#include <assert.h>

#endif
