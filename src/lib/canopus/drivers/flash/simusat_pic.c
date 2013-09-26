/*
 * flash empty implementation
 */
#include <canopus/drivers/flash.h>
#include <canopus/drivers/flash/ramcopy.h>

/* our POSIX 'fake' flash */
#define FLASH_SECTOR_COUNT 4
#define FLASH_SECTOR_SIZE_512   512
#define FLASH_SECTOR_SIZE_4K    (4*1024)

typedef char flash_data_t; // TODO uint16_t

size_t
flash_sector_wordsize(const void *addr)
{
    return sizeof(flash_data_t);
}

size_t
flash_sector_size(const void *flash_addr)
{
    return FLASH_SECTOR_SIZE_512;
}

flash_err_t _flash_init(void) {
    return FLASH_ERR_OK;
}
void _flash_init_end(void) {}

flash_err_t _flash_erase(const void *base_block_addr, unsigned int size_in_bytes, const vTaskDelay_t *_vTaskDelay) {
    char *d;
    int i;
    /* memset(base_block_addr, size_in_bytes, 0xff); */
    for (i = 0, d = (char*)base_block_addr; i < size_in_bytes; d++, i++) {
        *d = 0xff;
    }
    return FLASH_ERR_OK;
}
void _flash_erase_end(void) {}

flash_err_t _flash_write(const void *addr, const void *data, int len) {
    char *d, *s;
    int i;
    /* memcpy(addr, data, len); */
    for (i = 0, d = (char *)addr, s = (char *)data; i < len; i++) {
        d[i] = s[i];
    }
    return FLASH_ERR_OK;
}
void _flash_write_end(void) {}
