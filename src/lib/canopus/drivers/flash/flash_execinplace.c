#include <canopus/drivers/flash.h>
#include <canopus/drivers/flash/ramcopy.h>

#include <FreeRTOS.h>
#include <task.h>

#ifndef RAMCOPY_DISABLED
# warning RAM_COPY is DISABLED
#endif

flash_err_t flash_init(void) {
	return _flash_init();
}

flash_err_t flash_erase(const void *base_block_addr, unsigned int size_in_bytes) {
	return _flash_erase(base_block_addr, size_in_bytes, &vTaskDelay);
}

flash_err_t flash_write(const void *addr, const void *data, int len) {
	return _flash_write(addr, data, len);
}
