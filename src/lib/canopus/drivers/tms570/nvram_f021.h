#ifndef _CANOPUS_DRIVERS_HERCULES_NVRAM_F021_H
#define _CANOPUS_DRIVERS_HERCULES_NVRAM_F021_H

#include <canopus/types.h>

#define NVRAM_EEP_INDEX 0
#define NVRAM_BLOCK_NUMBER 1 /* first start at one */
#define NVRAM_BLOCK_SIZE 512 /* Fee_BlockConfiguration[NVRAM_BLOCK_NUMBER].FeeBlockSize */

retval_t flash_init_nvram(void);

#endif
