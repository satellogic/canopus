#ifndef _CANOPUS_SUBSYSTEM_MEMORY_H_
#define _CANOPUS_SUBSYSTEM_MEMORY_H_

#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/logging.h>
#include <canopus/md5.h>

#define MEM_READ_BLOCK_SIZE 200 // FIXME moved out?
#define DEFAULT_INTER_BULK_CMD_DELAY_ms		1500

extern subsystem_t SUBSYSTEM_MEMORY;

typedef struct nvram_header_t {
    unsigned char digest[MD5_DIGEST_SIZE];
    uint16_t version;
    uint16_t flash_access_count;
} nvram_header_t;

typedef struct nvram_mm_t {
    logmask_t logmask;
    uint16_t inter_bulk_cmd_delay_ms;
} nvram_mm_t;

// FIXME: Move to bootloader files
typedef struct nvram_bootloader_t {
} nvram_bootloader_t;

retval_t nvram_erase(void);
retval_t nvram_format_and_disable_flushing(uint32_t format_key, bool disable_flushing);
retval_t nvram_flush_partial(uint16_t offset, int16_t size);
void nvram_flush(void);
void nvram_reload(void);

retval_t MEMORY_nvram_save(const void *start, size_t size);
retval_t MEMORY_nvram_flush(void);
retval_t MEMORY_nvram_reload(void);

uintptr_t MEMORY_fw_start_address(void);

void *MEMORY_uploadarea_address(void);
size_t MEMORY_uploadarea_size(void);

#endif
