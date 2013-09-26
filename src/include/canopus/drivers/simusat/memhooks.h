#ifndef _CANOPUS_BOARD_MEMHOOKS_H
#define _CANOPUS_BOARD_MEMHOOKS_H

#include <canopus/types.h>

typedef enum memhooks_e {
	MEMHOOKS_NANOMIND,
	MEMHOOKS_TMS570LS3137
} memhooks_t;

void memhooks_init(memhooks_t type);
bool memhooks_flash_init(memhooks_t type);

#endif
