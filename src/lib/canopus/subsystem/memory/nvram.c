// TODO rename nvram_common_driver.c ?
#include <canopus/drivers/nvram.h>
#include <canopus/nvram.h>

#include <string.h>

nvram_t nvram; /* global */

extern const nvram_t nvram_default;

static uint16_t flash_access_count = 0; /* valid since last reboot */
static bool flushing_disabled = false;

static void
nv_copy(nvram_t *dst, const nvram_t *src)
{
    memcpy(dst, src, sizeof(nvram_t));
}

static void
nv_update_digest(nvram_t *nv)
{
    MD5_CTX ctx;

    memset(nv->hdr.digest, 0, sizeof(nv->hdr.digest));
    MD5Init(&ctx);
    MD5Update(&ctx, (unsigned char *)nv, sizeof(nvram_t));
    MD5Final(&ctx);
    //LOG_REPORT_MD5(LOG_NVRAM, "nv_update_digest: ", &ctx);

    memcpy(nv->hdr.digest, ctx.digest, sizeof(ctx.digest));
}

static bool
nv_is_version_valid(const nvram_t *nv)
{
    return NVRAM_VERSION_CURRENT == nv->hdr.version;
}

static bool
nv_is_digest_valid(const nvram_t *nv)
{
    nvram_t copy;
    MD5_CTX ctx;

    nv_copy(&copy, nv);

    memset(copy.hdr.digest, 0, sizeof(copy.hdr.digest));
    MD5Init(&ctx);
    MD5Update(&ctx, (unsigned char *)&copy, sizeof(nvram_t));
    MD5Final(&ctx);
    //LOG_REPORT_MD5(LOG_NVRAM, "nv_is_digest_valid: ", &ctx);

    return !memcmp(ctx.digest, nv->hdr.digest, sizeof(ctx.digest));
}

static bool
nv_is_valid(const nvram_t *nv, const char *desc)
{

    if (!nv_is_version_valid(nv)) {
        NVRAM_REPORT("nvram: [%s] wrong version (0x%04x)\n", desc, nv->hdr.version);
        return false;
    }

    if (!nv_is_digest_valid(nv)) {
        NVRAM_REPORT("nvram: [%s] incorrect md5\n", desc);
        return false;
    }

    NVRAM_REPORT("nvram: [%s] valid!\n", desc);

    return true;
}

static void
nv_fill_valid(nvram_t *nv, const char *desc)
{
    nvram_t nvbuf;

    #if 0
    // XXX do we want to restore nvram?
    if (&nvram == nv) {
        return;
    }
    #endif

    /* check ram */
    if (nv_is_valid(&nvram, "RAM")) {
        NVRAM_REPORT("nvram_fill [%s]: ram correct!\n", desc);
        nv_copy(nv, &nvram);
        return;
    }

    /* try restore from flash */
    if (RV_SUCCESS == nvram_read_blocking((uint8_t *)&nvbuf, sizeof(nvbuf))) {
        if (nv_is_valid(&nvbuf, "FLASH")) {
            NVRAM_REPORT("nvram_fill [%s]: nvram0/flash correct! restoring\n", desc);
            nv_copy(nv, &nvbuf);
            return;
        }
    }

    /* restore from .rodata */
    NVRAM_REPORT("nvram_fill [%s]: reset to default (.rodata)\n", desc);

    nv_copy(nv, &nvram_default);
    nv_update_digest(nv); /* because nv->hdr.digest[] == { } in .rodata */
}

static retval_t
nv_are_all_values_correct(const nvram_t *nv)
{
    retval_t rv = RV_SUCCESS;

    FUTURE_HOOK_2(nv_ram_verify_good_values_0, nv, &rv);
    if (RV_SUCCESS == rv) {
        // TODO
    }
    FUTURE_HOOK_2(nv_ram_verify_good_values_1, nv, &rv);

    NVRAM_REPORT("nvram: values returned %s\n", retval_s(rv));

    return rv;
}

static retval_t
nv_save_flash(const nvram_t *nv)
{
    return nvram_write_blocking((uint8_t *)nv, sizeof(*nv));
}

static retval_t
nv_save(const nvram_t *nv, const char *desc)
{
    nvram_t copy;

    if (flushing_disabled) {
        NVRAM_REPORT("nvram_save [%s]: flushing disabled\n", desc);
        return RV_PERM;
    }

    if (RV_SUCCESS != nv_are_all_values_correct(nv)) {
        NVRAM_REPORT("nvram_save [%s]: invalid values\n", desc);
        return RV_ILLEGAL;
    }

    NVRAM_REPORT("nvram_save [%s]: access_count=%d\n", desc, flash_access_count);

    nv_copy(&copy, nv);
    copy.hdr.flash_access_count = flash_access_count++;

    nv_update_digest(&copy);

    return nv_save_flash(&copy);
}

// FIXME scheduler critical lock?
void
nvram_reload()
{
    NVRAM_REPORT("==nvram_reload\n");
    nv_fill_valid(&nvram, "RAM");
}

// FIXME scheduler critical lock?
retval_t
nvram_flush_partial(uint16_t offset, int16_t size)
{
    nvram_t copy;

    NVRAM_REPORT("==nvram_flush_partial\n");
    if (offset + size > sizeof(nvram_t)) {
        return RV_ILLEGAL;
    }

    /* fill with previous */
    nv_fill_valid(&copy, "copy");

    /* update with current */
    memcpy((char *)&copy + offset, (const char *)&nvram + offset, size);

    return nv_save(&copy, "copy");
}

// FIXME scheduler critical lock?
void
nvram_flush()
{
    NVRAM_REPORT("==nvram_flush\n");
    (void)nv_save(&nvram, "RAM");
}

retval_t
nvram_erase()
{
	return nvram_erase_blocking();
}

retval_t
nvram_format_and_disable_flushing(uint32_t format_key, bool disable_flushing)
{
	flushing_disabled = disable_flushing;

	return nvram_format_blocking(format_key);
}
