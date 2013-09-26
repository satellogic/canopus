#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/flash.h>
#include <string.h>

#ifdef FLASH_DEBUG
const char *
dev_flash_errmsg(flash_err_t err) {
	switch (err) {
	case FLASH_ERR_OK:
		return "No error - operation complete";
	case FLASH_ERR_ERASE_SUSPEND:
		return "Device is in erase suspend state";
	case FLASH_ERR_PROGRAM_SUSPEND:
		return "Device is in program suspend state";
	case FLASH_ERR_INVALID:
		return "Invalid FLASH address";
	case FLASH_ERR_ERASE:
		return "Error trying to erase";
	case FLASH_ERR_LOCK:
		return "Error trying to lock/unlock";
	case FLASH_ERR_PROGRAM:
		return "Error trying to program";
	case FLASH_ERR_PROTOCOL:
		return "Generic error";
	case FLASH_ERR_PROTECT:
		return "Device/region is write-protected";
	case FLASH_ERR_NOT_INIT:
		return "FLASH sub-system not initialized";
	case FLASH_ERR_DRV_VERIFY:
		return "Data verify failed after operation";
	case FLASH_ERR_DRV_TIMEOUT:
		return "Driver timed out waiting for device";
	case FLASH_ERR_DRV_WRONG_PART:
		return "Driver does not support device";
	case FLASH_ERR_LOW_VOLTAGE:
		return "Device reports low voltage";
	default:
		return "Unknown error";
	}
}
#endif /* FLASH_DEBUG */

/* Definition of "Sector":
 * a contiguous region of Flash memory which must be erased simultaneously. */

#define FLASH_SECTOR_MAXSIZE (1*1024) /* FIXME nano:64K,8K tms: 128K,32K,16K */

// TODO arch-dep: TMS570 poweron dedicated SRAM bank, use it, power it off
static char *
flash_alloc(size_t size) /* may come from HEAP, BSS or another free sector */
{
    static char bss_buf[FLASH_SECTOR_MAXSIZE]; /* XXX not reentrant */

    assert(size <= FLASH_SECTOR_MAXSIZE);

    return bss_buf;
}

static void
flash_free(const char *ptr)
{
    /* nothing */
}

#define FLASH_WORD_WRITABLE(current, new) (((current) & (new)) == new)

static enum { NO, YES }
sector_needs_erase(const void *dst, const void *src, size_t size)
{
    size_t wordsize = flash_sector_wordsize(dst);

    if (wordsize == sizeof(uint16_t)) {
        uint16_t *d, *s;

        size /= sizeof(uint16_t);
        for (d = (uint16_t *)dst, s = (uint16_t *)src; size; size--, d++, s++) {
            if (!FLASH_WORD_WRITABLE(*d, *s)) {
                return YES;
            }
        }
    } else {
        uint8_t *d, *s;

        for (d = (uint8_t *)dst, s = (uint8_t *)src; size; size--, d++, s++) {
            if (!FLASH_WORD_WRITABLE(*d, *s)) {
                return YES;
            }
        }
    }

    return NO;
}

#define FLASH_SECTOR_MASK(addr) (flash_sector_size(addr) - 1) /* expect PAGE aligned sector_size */

static flash_err_t
flash_patch_sector(const void *dst, const void *src, size_t size, flashpatch_mode_e mode)
{
    const size_t sector_size = flash_sector_size(dst);
    char *buf = (char *)src;
    flash_err_t err;

    /* verify internal library calls correctness (must not happen) */
    assert(size > 0);
    assert(size <= sector_size);

    if (mode != FLASHPATCH_FORCE && !memcmp(dst, src, size)) {
        /* no need to write the flash if the data is already here
         * except if we "force" the write */
        return FLASH_ERR_OK;
    }

    /* if there is enough bits high we might avoid an Erase */
    if (sector_needs_erase(dst, src, size) == YES) {
        uintptr_t sector_baseaddr, sector_mask;
        off_t sector_offset;

        if (mode == FLASHPATCH_NOERASE) {
            return FLASH_ERR_ERASE;
        }
        sector_mask = FLASH_SECTOR_MASK(dst);
        sector_baseaddr = ((uintptr_t)dst) & ~sector_mask;
        sector_offset = ((uintptr_t)dst) & sector_mask;
        if ((buf = flash_alloc(sector_size)) == NULL) {
            return FLASH_ERR_PROTOCOL;
        }

        if (sector_offset + size > sector_size) {
            size = sector_size - sector_offset; /* avoid buffer overflow */
        }
        /* copy original flash data, then insert our data */
        memcpy(buf, (const void *)sector_baseaddr, sector_size);
        memcpy(buf + sector_offset, src, size);

        dst = (void *)sector_baseaddr;
        size = sector_size;
        err = flash_erase(dst, size);
        if (FLASH_ERR_OK != err) {
            flash_free(buf);
            return err;
        }
    }
    err = flash_write(dst, buf, size);

    if (buf != (char *)src) {
        flash_free(buf);
    }

    return err;
}

flash_err_t
flash_patch_ex(const void *dst, const void *src, size_t size, flashpatch_mode_e mode) {
    uintptr_t dst_addr, src_addr;
    flash_err_t err;

    dst_addr = (uintptr_t)dst;
    src_addr = (uintptr_t)src;
    while (size) { /* for each sector */
        size_t wr_size, sector_size;

        sector_size = flash_sector_size((void*)dst_addr);
        if (size > sector_size) {
            wr_size = sector_size;
        } else {
            wr_size = size;
        }
        err =  flash_patch_sector((void*)dst_addr, (void*)src_addr, wr_size, mode);
        if (FLASH_ERR_OK != err) {
            return err;
        }
        size -= wr_size;
        dst_addr += wr_size;
        src_addr += wr_size;
    }

    return FLASH_ERR_OK;
}

flash_err_t
flash_patch(const void *dst, const void *src, size_t size)
{
    return flash_patch_ex(dst, src, size, FLASHPATCH_NORMAL);
}

flash_err_t
flash_patch_force(const void *dst, const void *src, size_t size)
{
    return flash_patch_ex(dst, src, size, FLASHPATCH_FORCE);
}
