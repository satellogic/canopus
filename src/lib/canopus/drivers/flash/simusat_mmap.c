#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/simusat/memhooks.h>
#include <canopus/drivers/flash.h>
#include <canopus/drivers/flash/at49b320d.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#if 0
# include <canopus/logging.h>
# define FLASH_REPORT(args...) log_report_fmt(LOG_FLASH, args)
#else
# include <stdio.h>
# define FLASH_REPORT(args...) printf(args)
#endif


typedef struct flash_cfg flash_cfg_t;
typedef struct flash_ctx flash_ctx_t;

struct flash_cfg {
    char *name;
    size_t wordsize;
    size_t size;
    const void *(*sector_base_addr)(flash_ctx_t *ctx, const void *addr);
    size_t (*sector_size)(flash_ctx_t *ctx, const void *addr);
    flash_err_t (*init)(flash_ctx_t *ctx);
    flash_err_t (*sector_erase)(flash_ctx_t *ctx, const void *addr, unsigned int size_in_bytes);
    flash_err_t (*sector_write)(flash_ctx_t *ctx, const void *addr, const void *data, int len);
    int prot;
};

struct flash_ctx {
    const flash_cfg_t *cfg;
    int fd;
    off_t size;
    void *m;
};

#define FLASH_MAX 4

/* ==================== mmio dumb impl ==================== */

static bool
flash_mmio_enable_write(flash_ctx_t *f)
{
    return !mprotect(f->m, f->size, f->cfg->prot | PROT_WRITE);
}

static bool
flash_mmio_disable_write(flash_ctx_t *f)
{
    if (mprotect(f->m, f->size, f->cfg->prot & ~PROT_WRITE)) {
        return false;
    }
    return !msync(f->m, f->size, MS_SYNC);
}

flash_err_t
flash_mmio_dumb_init(flash_ctx_t *ctx)
{
    return FLASH_ERR_OK;
}

flash_err_t
flash_mmio_dumb_sector_erase(flash_ctx_t *ctx, const void *addr, unsigned int size)
{
    assert(size <= ctx->cfg->sector_size(ctx, ctx->cfg->sector_base_addr(ctx, addr)));

    flash_mmio_enable_write(ctx);
    memset((void *)addr, 0xff, size);
    flash_mmio_disable_write(ctx);

    return FLASH_ERR_OK;
}

flash_err_t
flash_mmio_dumb_sector_write(flash_ctx_t *ctx, const void *addr, const void *ptr, int size)
{
    assert(size <= ctx->cfg->sector_size(ctx, ctx->cfg->sector_base_addr(ctx, addr)));

    flash_mmio_enable_write(ctx);
    memcpy((void *)addr, ptr, size);
    flash_mmio_disable_write(ctx);

    return FLASH_ERR_OK;
}

/* ==================== internal impl ==================== */

static flash_ctx_t flash[FLASH_MAX] = { };

static flash_ctx_t *
empty_flash_ctx()
{
	int i;

	for (i = 0; i < ARRAY_COUNT(flash); i++) {
		if (NULL == flash[i].cfg) {
			return &flash[i];
		}
	}
	return NULL;
}

static flash_ctx_t *
flash_ctx(const void *addr)
{
#ifdef NANOMIND
    return &flash[((uintptr_t)addr >> 27) & 1]; // XXX nanomind only
#else
	uintptr_t uaddr = (uintptr_t)addr;
	int i;

	for (i = 0; i < ARRAY_COUNT(flash); i++) {
		uintptr_t baddr = (uintptr_t)flash[i].m;

		if (baddr <= uaddr && uaddr < baddr + flash[i].size) {
			return &flash[i];
		}
	}
	return NULL;
#endif
}

static bool
mmap_flash_init(uintptr_t addr, const char *pathname, const flash_cfg_t *cfg)
{
    struct stat st;
    size_t length = cfg->size;
    off_t offset = 0;

    flash_ctx_t *f = empty_flash_ctx();
    assert(NULL != f);
    f->fd = open(pathname, O_RDWR|O_CREAT|O_SYNC, 0644);
    if (-1 == f->fd) return false;
    if (-1 == fstat(f->fd, &st)) return false;
    if (st.st_size < cfg->size) {
        char c = 0xff;

#ifdef INIT_FLASH_FILE
        int i;

        if ((off_t) -1 == lseek(f->fd, st.st_size, SEEK_SET)) return false;
        for (i = st.st_size; i < length; i++) {
            write(f->fd, &c, sizeof(c));
        }
#else
        if ((off_t) -1 == lseek(f->fd, cfg->size - sizeof(c), SEEK_SET)) return false;
        write(f->fd, &c, sizeof(c));
#endif
    }
    if (0 == addr) { /* can not map NULL anymore ;) */
    	offset = sysconf(_SC_PAGE_SIZE);

    	if (offset < 0x10000) {
    		offset = 0x10000;
    	}
    	addr += offset;
    	length -= offset;
    }
    f->m = mmap((void *)addr, length, cfg->prot, MAP_SHARED|MAP_FIXED, f->fd, offset);
    if ((void *)addr != f->m) {
        FLASH_REPORT("0x%08lx (size:0x%06x) ERROR initializing %s\n", addr, length, cfg->name);
    	return false;
    }

    f->cfg = cfg;
    f->size = length;
    FLASH_REPORT("0x%08lx (size:0x%06x) initialized as %s using %s\n", addr, length, cfg->name, pathname);

    return true;
}

/* ==================== external impl ==================== */

size_t
flash_sector_wordsize(const void *addr)
{
    flash_ctx_t *f = flash_ctx(addr);

    if (NULL == f) return 0;

    return f->cfg->wordsize;
}

size_t
flash_sector_size(const void *flash_addr)
{
    flash_ctx_t *f = flash_ctx(flash_addr);

    if (NULL == f) return 0;

    return f->cfg->sector_size(f, flash_addr);
    //return f->cfg->sector_size((ptrdiff_t)flash_addr - (ptrdiff_t)f->m);
}

flash_err_t
_flash_init(void) // FIXME this is board_flash_init()
{
    int i;
    flash_err_t rv = FLASH_ERR_OK;

    for (i = 0; i < ARRAY_COUNT(flash); i++) {
        if (NULL != flash[i].cfg) {
            if (FLASH_ERR_OK != flash[i].cfg->init(&flash[i])) {
                rv = FLASH_ERR_NOT_INIT;
            }
        }
    }

    return rv;
}

flash_err_t
_flash_erase(const void *addr, unsigned int size)
{
    flash_ctx_t *f = flash_ctx(addr);

    /* word aligned */
    if (size % 1) size += 1;

    // TODO align sector addr/size

    FLASH_REPORT("flash_erase(0x%08x, 0x%06x)\n", addr, size);
    return f->cfg->sector_erase(f, addr, size);
}

flash_err_t
_flash_write(const void *addr, const void *ptr, int size)
{
    flash_ctx_t *f = flash_ctx(addr);

    /* word aligned */
    if (size % 1) size += 1;

    // TODO align sector addr/size

    FLASH_REPORT("flash_write(0x%08x, %p, 0x%06x)\n", addr, ptr, size);
    return f->cfg->sector_write(f, addr, ptr, size);
}

static const void *
TODO_sector_base_addr(flash_ctx_t *ctx, const void *addr)
{
    return addr; // XXX TODO
}

/* ==================== nanomind simulation ==================== */

static size_t
at49bv320dt_sector_size(flash_ctx_t *ctx, const void *addr)
{
	flash_data_t *block_base = (flash_data_t *)addr;

	if (IS_64k_SECTOR(block_base)) {
        return FLASH_64k_BLOCK_SIZE;
    }
    else if (IS_8k_SECTOR(block_base)) {
        return FLASH_8k_BLOCK_SIZE;
    }

    /* error */
    return 0;
}

static const flash_cfg_t at49bv320dt_cfg = {
    "AT49BV320DT",
    sizeof(uint16_t),
    4*1024*1024,
    TODO_sector_base_addr,
    at49bv320dt_sector_size,
    flash_mmio_dumb_init,
    flash_mmio_dumb_sector_erase,
    flash_mmio_dumb_sector_write,
    PROT_EXEC|PROT_READ
};

static bool
memhooks_nanomind_flash_init(void)
{
    (void)mmap_flash_init(0x40000000UL, "FLASH0.bin", &at49bv320dt_cfg);
    (void)mmap_flash_init(0x48000000UL, "FLASH1.bin", &at49bv320dt_cfg);

    return true;
}

/* ==================== tms570ls3137 simulation ==================== */

static size_t
tms570ls3137_sector_size(flash_ctx_t *ctx, const void *addr)
{
	uintptr_t baddr = (uintptr_t)ctx->m;

	if (baddr >= 0xf0200000UL) {
		return 16*1024;
	} else if (baddr < 0x00020000UL) {
		return 32*1024;
	} else {
		return 128*1024;
	}
}

static const flash_cfg_t f021_cfg[] = {
    [0] = {
        "F021 bank#0",
        sizeof(uint16_t), // FIXME
        12*128*1024,
        TODO_sector_base_addr, // XXX
        tms570ls3137_sector_size,
        flash_mmio_dumb_init,
        flash_mmio_dumb_sector_erase,
        flash_mmio_dumb_sector_write,
        PROT_EXEC|PROT_READ
    },
    [1] = {
        "F021 bank#1",
        sizeof(uint16_t), // FIXME
        12*128*1024,
        TODO_sector_base_addr, // XXX
        tms570ls3137_sector_size,
        flash_mmio_dumb_init,
        flash_mmio_dumb_sector_erase,
        flash_mmio_dumb_sector_write,
        PROT_EXEC|PROT_READ
    },
    [7] = {
        "F021 bank#7",
        sizeof(uint16_t), // FIXME
        4*16*1024,
        TODO_sector_base_addr, // XXX
        tms570ls3137_sector_size,
        flash_mmio_dumb_init,
        flash_mmio_dumb_sector_erase,
        flash_mmio_dumb_sector_write,
        PROT_READ
    }
};

static bool
memhooks_tms570ls3137_flash_init(void)
{
    (void)mmap_flash_init(0x00000000UL, "FLASH0.bin", &f021_cfg[0]);
    (void)mmap_flash_init(0x00180000UL, "FLASH1.bin", &f021_cfg[1]);
    (void)mmap_flash_init(0xf0200000UL, "FLASH7.bin", &f021_cfg[7]);

    return true;
}

bool
memhooks_flash_init(memhooks_t type)
{
	switch (type) {
	case MEMHOOKS_NANOMIND:
		return memhooks_nanomind_flash_init();
	case MEMHOOKS_TMS570LS3137:
		return memhooks_tms570ls3137_flash_init();
	default:
		return false;
	}
}
