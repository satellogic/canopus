#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/flash.h>
#include <canopus/drivers/nvram.h>

#include <string.h>

extern const char _nvram0_addr; /* linkscript provided */
#define nvram0 ((const nvram_t *)&_nvram0_addr)

retval_t
nvram_read_blocking(void *addr, size_t size)
{
	memcpy(addr, nvram0, sizeof(nvram_t));

	return RV_SUCCESS;
}

retval_t
nvram_write_blocking(const void *addr, size_t size)
{
    // TODO this is too aggressive! improve...

    NVRAM_REPORT("nvram: flash_erase\n");
    (void )flash_erase(nvram0, sizeof(nvram_t));

    NVRAM_REPORT("nvram: flash_write\n");
    if (FLASH_ERR_OK != flash_write(nvram0, addr, sizeof(nvram_t))) {
        return RV_ERROR;
    }

    return RV_SUCCESS;
}

retval_t
nvram_erase_blocking()
{
    return RV_NOTIMPLEMENTED;
}

retval_t
nvram_format_blocking(uint32_t format_key)
{
    return RV_NOTIMPLEMENTED;
}
