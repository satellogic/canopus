/* memory arrays to simulate fixed addressing (see linkscripts) */
#include <stdint.h>
#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/simusat/memhooks.h>

#if defined(_POSIX_SOURCE)
# include <sys/mman.h>
#endif

#if 0
# include <canopus/logging.h>
# define MEMHOOKS_REPORT(args...) log_report_fmt(LOG_GLOBAL, args)
#else
# include <stdio.h>
# define MEMHOOKS_REPORT(args...) printf(args)
#endif

#ifndef MEMHOOKS_ADDR_PROVIDED_BY_LINKSCRIPT
/* FLASH */
uint16_t __stage_0_config__[0x4000];

uint16_t _nvram0_addr[8*1024];
uint16_t _nvram1_addr[8*1024];
uint16_t _cdh_sample_addr[8*1024];
#endif

/* DRAM */
#define UPLOAD_MEMORY_AREA_SIZE (1*1024*1024)
char upload_memory_area_start[UPLOAD_MEMORY_AREA_SIZE], upload_memory_area_stop[UPLOAD_MEMORY_AREA_SIZE];

static bool
mmap_ram_init(uintptr_t addr, size_t size, const char *type, void **ptr)
{
    void *m = NULL;

#if defined(_POSIX_SOURCE)
    // FIXME win32?
    m = mmap((void *)addr, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
    if ((void *)addr != m) return false;
#else
    #warning "!POSIX MMAP not implemented!"
    puts("!POSIX MMAP not implemented! expect SEGV soon...");
#endif

    if (NULL != ptr) *ptr = m;
    MEMHOOKS_REPORT("0x%08lx (size:0x%06x) initialized as %s\n", addr, size, type);

    return true;
}

static void *sram = NULL, *dram = NULL;

static void
memhooks_nanomind_ram_init(void)
{
    assert(mmap_ram_init(0x00300000UL, 4*1024, "SRAM", &sram));
    assert(mmap_ram_init(0x50000000UL, 2*1024*1024, "DRAM", &dram));
}

static void
memhooks_tms570ls3137_ram_init(void)
{
    assert(mmap_ram_init(0x08000000UL, 4*64*1024, "SRAM", &sram));
}

static void
memhooks_nanomind_init(void)
{
	memhooks_nanomind_ram_init();
	assert(memhooks_flash_init(MEMHOOKS_NANOMIND));
}

static void
memhooks_tms570ls3137_init(void)
{
	memhooks_tms570ls3137_ram_init();
	assert(memhooks_flash_init(MEMHOOKS_TMS570LS3137));
}

void
memhooks_init(memhooks_t type)
{
	switch (type) {
	case MEMHOOKS_NANOMIND:
		memhooks_nanomind_init();
		break;
	case MEMHOOKS_TMS570LS3137:
		memhooks_tms570ls3137_init();
		break;
	}
}

/* MM */
#ifdef __MINGW32__
extern int main(int, char **);
const void *_start = &main;
#endif
