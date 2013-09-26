#include <stdint.h>
#include <string.h>

#include <canopus/drivers/flash.h>
#include <canopus/drivers/flash/ramcopy.h>
#include <canopus/assert.h>

#include <FreeRTOS.h>
#include <task.h>

#ifndef RAMCOPY_DISABLED

flash_err_t flash_init(void) {
	uint8_t buffer_in_ram[DEV_FLASH_INIT_LEN];
	flash_init_t *function_in_ram = (flash_init_t*)&buffer_in_ram;

    FUTURE_HOOK_0(ramcopy_flash_init);
	assert(DEV_FLASH_INIT_LEN > 0);
	memcpy(function_in_ram, &_flash_init, sizeof(buffer_in_ram));

	return function_in_ram();
}

flash_err_t flash_erase(const void *base_block_addr, unsigned int size_in_bytes) {
	uint8_t buffer_in_ram[DEV_FLASH_ERASE_LEN];
	flash_erase_t *function_in_ram = (flash_erase_t*)&buffer_in_ram;

    FUTURE_HOOK_2(ramcopy_flash_erase, &base_block_addr, &size_in_bytes);
	assert(DEV_FLASH_ERASE_LEN > 0);
	memcpy(function_in_ram, &_flash_erase, sizeof(buffer_in_ram));

	return function_in_ram(base_block_addr, size_in_bytes, &vTaskDelay);
}

flash_err_t flash_write(const void *addr, const void *data, int len) {
	uint8_t buffer_in_ram[DEV_FLASH_WRITE_LEN];
	flash_write_t *function_in_ram = (flash_write_t*)&buffer_in_ram;

    FUTURE_HOOK_3(ramcopy_flash_write, &addr, &data, &len);
	assert(DEV_FLASH_WRITE_LEN > 0);
	memcpy(function_in_ram, &_flash_write, sizeof(buffer_in_ram));

	return function_in_ram(addr, data, len);
}

#else /* RAMCOPY_DISABLED */
#warning RAM_COPY is DISABLED

flash_err_t flash_init(void) {
	return _flash_init();
}

flash_err_t flash_erase(const void *base_block_addr, unsigned int size_in_bytes) {
	return _flash_erase(base_block_addr, size_in_bytes, &vTaskDelay);
}

flash_err_t flash_write(const void *addr, const void *data, int len) {
	return _flash_write(addr, data, len);
}

#endif /* RAMCOPY_DISABLED */
