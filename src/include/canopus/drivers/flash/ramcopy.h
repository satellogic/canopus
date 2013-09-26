#ifndef _CANOPUS_DRIVERS_FLASH_RAMCOPY_H
#define _CANOPUS_DRIVERS_FLASH_RAMCOPY_H

#include <canopus/drivers/flash.h>

#include <FreeRTOS.h>

typedef void vTaskDelay_t( portTickType xTicksToDelay);

typedef flash_err_t flash_init_t(void);
typedef flash_err_t flash_erase_t(const void *base_block_addr, unsigned int size_in_bytes, const vTaskDelay_t *_vTaskDelay);
typedef flash_err_t flash_write_t(const void *addr, const void *data, int len);
typedef void flash_function_delimiter_t(void);

flash_init_t _flash_init;
flash_erase_t _flash_erase;
flash_write_t _flash_write;
flash_function_delimiter_t _flash_init_end, _flash_erase_end, _flash_write_end;

#define _FLASH_FUNCTIONCODE_SIZE(fname) (((char*)&_flash_##fname##_end)-((char*)&_flash_##fname))
#define DEV_FLASH_INIT_LEN  _FLASH_FUNCTIONCODE_SIZE(init)
#define DEV_FLASH_ERASE_LEN	_FLASH_FUNCTIONCODE_SIZE(erase)
#define DEV_FLASH_WRITE_LEN	_FLASH_FUNCTIONCODE_SIZE(write)

#endif
