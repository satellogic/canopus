#ifndef _CANOPUS_DRIVERS_FLASH_H
#define _CANOPUS_DRIVERS_FLASH_H

#include <stdint.h>
#include <canopus/types.h>

typedef uint8_t flash_err_t;
typedef const void *flash_addr_t;

#define FLASH_ERR_OK              0x00  /* No error - operation complete */
#define FLASH_ERR_INVALID         0x01  /* Invalid FLASH address */
#define FLASH_ERR_ERASE           0x02  /* Error trying to erase */
#define FLASH_ERR_LOCK            0x03  /* Error trying to lock/unlock */
#define FLASH_ERR_PROGRAM         0x04  /* Error trying to program */
#define FLASH_ERR_PROTOCOL        0x05  /* Generic error */
#define FLASH_ERR_PROTECT         0x06  /* Device/region is write-protected */
#define FLASH_ERR_NOT_INIT        0x07  /* FLASH info not yet initialized */
#define FLASH_ERR_HWR             0x08  /* Hardware (configuration?) problem */
#define FLASH_ERR_ERASE_SUSPEND   0x09  /* Device is in erase suspend mode */
#define FLASH_ERR_PROGRAM_SUSPEND 0x0a  /* Device is in in program suspend mode */
#define FLASH_ERR_DRV_VERIFY      0x0b  /* Driver failed to verify data */
#define FLASH_ERR_DRV_TIMEOUT     0x0c  /* Driver timed out waiting for device */
#define FLASH_ERR_DRV_WRONG_PART  0x0d  /* Driver does not support device */
#define FLASH_ERR_LOW_VOLTAGE     0x0e  /* Not enough juice to complete job */

size_t flash_sector_wordsize(const void *addr);
size_t flash_sector_size(const void *addr);

typedef enum flashpatch_mode_e {
    FLASHPATCH_NORMAL,
    FLASHPATCH_FORCE,
    FLASHPATCH_NOERASE,
} flashpatch_mode_e;

flash_err_t flash_init(void);
flash_err_t flash_erase(const void *base_block_addr, unsigned int size_in_bytes); /* FIXME sector_erase */
flash_err_t flash_write(const void *addr, const void *data, int len); /* FIXME sector_write */

/* (eventually unaligned) (eventually bigger than a sector) flash write */
flash_err_t flash_patch(const void *dst, const void *src, size_t size);
flash_err_t flash_patch_force(const void *dst, const void *src, size_t size);
flash_err_t flash_patch_ex(const void *dst, const void *src, size_t size, flashpatch_mode_e mode);

#ifdef FLASH_DEBUG
const char *dev_flash_errmsg(flash_err_t err);
#endif

#endif
