#ifndef _CANOPUS_ARCHDEFS_H
#define _CANOPUS_ARCHDEFS_H

// FIXME do we want to include <stddef.h> here?

/*
 * compiler: TI Hercules
 */
#if defined(__TMS470__) || defined(__TI_ARM__)
# define BOOL_TYPE_MISSING
# define TIMESPEC_TYPE_MISSING
# define RAMCOPY_DISABLED // XXX
# define VARARGS_SUPPORTED
# define DEBUG_CONSOLE_ENABLED /* XXX this should be <board_defs.h> */

/*
 * compiler: gcc
 */
#elif defined(__GNUC__)
# include <features.h>
# define BOOL_TYPE_MISSING
# define VARARGS_SUPPORTED
# define DEBUG_CONSOLE_ENABLED
# define BINARY_PREFIX_SUPPORTED /* http://gcc.gnu.org/onlinedocs/gcc/Binary-constants.html */

# if defined(__arm__)
   /*
    * GomSpace Nanomind
    */
#  if defined(__ARMEL__) && defined(__ARM_ARCH_4T__)
#   define THUMB_INTERWORK
#   define PACK_STRUCT_END __attribute((packed))
#   define ALIGN_STRUCT_END __attribute((aligned(4)))
#  endif
  /*
   * simusat
   */
# elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
#  if defined(__linux__)
#   define TIMESPEC_TYPE_MISSING
#  elif 0 && defined(__MINGW32__) /* __WIN32__ */
#   define TIMESPEC_TYPE_MISSING
#  elif 0 && defined(__MINGW64__)
#  else
#   error OS not yet supported
#  endif
#  define RAMCOPY_DISABLED // XXX
#  ifndef MEMHOOKS_ADDR_PROVIDED_BY_LINKSCRIPT // XXX
#   define MEMHOOKS_ADDR_PROVIDED_BY_LINKSCRIPT
#  endif
# endif

/*
 * compiler: none
 */
#else
# error "compiler not *yet* supported"
# define BOOL_TYPE_MISSING
# define _POSIX_SOURCE
# define __GNUC__
# define DEBUG_CONSOLE_ENABLED
#endif

/* Eclipse CDT parser (CODAN) */
#ifndef __FILE__ // TODO would be easier to check a define like __CDT_PARSER__
# define __FILE__ ""
#endif
#ifndef __LINE__
# define __LINE__ 0
#endif

#endif
