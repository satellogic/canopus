#ifndef _CANOPUS_DRIVERS_NVRAM_H
#define _CANOPUS_DRIVERS_NVRAM_H

#include <stdint.h>
#include <canopus/types.h>
#include <canopus/nvram.h> /* nvram_t */

#if 1
# include <canopus/logging.h>
# define NVRAM_REPORT(args...) log_report_fmt(LOG_NVRAM, args)
#else
# define NVRAM_REPORT(args...)
#endif

/* board/arch/app-dependant nvram access */
retval_t nvram_erase_blocking(void);
retval_t nvram_format_blocking(uint32_t format_key);
retval_t nvram_read_blocking(void *addr, size_t size);
retval_t nvram_write_blocking(const void *addr, size_t size);

#endif
