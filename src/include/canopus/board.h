#ifndef _CANOPUS_BOARD_H
#define _CANOPUS_BOARD_H

#include <canopus/types.h>

/** Board initialization: MUST be called before being able to make
 *  sensible use of values exported from this file, unless otherwise
 *  explicitly noted */
extern void board_init_scheduler_not_running(void);

extern retval_t board_init_scheduler_running(void);

#endif
