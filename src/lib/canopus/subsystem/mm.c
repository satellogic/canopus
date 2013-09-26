#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/mm.h>
#include <canopus/subsystem/platform.h>
#include <canopus/subsystem/cdh.h>
#include <canopus/watchdog.h>
#include <canopus/md5.h>
#include <canopus/logging.h>
#include <canopus/drivers/flash.h>
#include <canopus/nvram.h>

#include <string.h>

#include "memory/string.h"
#include "memory/compression.h"

retval_t
MEMORY_nvram_save(const void *start, size_t size)
{
    if (start < (void *)&nvram) {
        return RV_ILLEGAL;
    }
    if ((uintptr_t)start + size >= (uintptr_t)&nvram + sizeof(nvram)) {
        return RV_ILLEGAL;
    }
    return nvram_flush_partial((uintptr_t)start - (uintptr_t)&nvram, size);
}

retval_t
MEMORY_nvram_flush()
{
    nvram_flush();

    return RV_SUCCESS;
}

retval_t
MEMORY_nvram_reload() {
    nvram_reload();

    return RV_SUCCESS;
}

uintptr_t
MEMORY_fw_start_address()
{
    extern const void *_start;

    return (uintptr_t)&_start;
}

void *
MEMORY_uploadarea_address()
{
    extern char upload_memory_area_start;
    return (void *)&upload_memory_area_start;
}

size_t
MEMORY_uploadarea_size()
{
    extern char upload_memory_area_start, upload_memory_area_stop;

    return (ptrdiff_t)&upload_memory_area_stop - (ptrdiff_t)&upload_memory_area_start;
}

static void mm_mode_from_booting_to_initialize(const subsystem_t *ss) {
	/* ToDo: Proper initialization.
	 *  CAN'T BLOCK
	 *  read nvram from flash
	 *  check integrity of nvram
	 */

	bool success = true;

//	success &= FLASH_ERR_OK == flash_init();

	PLATFORM_ss_is_ready(ss);
}

static void MEMORY_main_task(void *pvParameters) {
	satellite_mode_e mode = SM_BOOTING;
	satellite_mode_e prev_mode;

    subsystem_t * ss = (subsystem_t *)pvParameters;

	PLATFORM_ss_is_ready(ss);

    while (1) {
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode, PM_MODE_CHANGE_DETECT_TIME_SLOT_MS / portTICK_RATE_MS)) {
			prev_mode = PLATFORM_previous_satellite_mode();
			FUTURE_HOOK_3(memory_mode_change, ss, &prev_mode, &mode);

			if (prev_mode == SM_BOOTING && mode == SM_INITIALIZING) {
				mm_mode_from_booting_to_initialize(ss);
			} else {
				PLATFORM_ss_is_ready(ss);
			}
		} else {
			// No mode change. Tick
			PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);

			switch (mode) {
			case SM_OFF:
			case SM_BOOTING:
			case SM_INITIALIZING:
			case SM_SURVIVAL:
			case SM_MISSION:
			case SM_LOW_POWER:
				break;
			case SM_COUNT:
				break;
			}
		}
	}
}

static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_put_u32(oframe, xPortGetFreeHeapSize());
    FUTURE_HOOK_2(mm_cmd_get_telemetry_beacon, iframe, oframe);

	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    FUTURE_HOOK_2(mm_cmd_get_telemetry_beacon_short, iframe, oframe);

	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return RV_SUCCESS;
}

static retval_t cmd_mem_read(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *src=0;

	if (RV_SUCCESS != frame_get_u32(iframe, (void*)&src)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_put_data(oframe, src, MEM_READ_BLOCK_SIZE)) return RV_NOSPACE;
	return RV_SUCCESS;
}

static retval_t cmd_mem_read_long(const subsystem_t *self, frame_t * iframe, frame_t * oframe, uint32_t seqnum) {
	void *src=0;
	frame_t *cmd_frame;
	uint32_t size;
	retval_t rv;

	if (RV_SUCCESS != frame_get_u32(iframe, (void*)&src)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &size)) return RV_NOSPACE;

	if (RV_SUCCESS != frame_put_u32(oframe, (uintptr_t)src)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_put_data(oframe, src, MEM_READ_BLOCK_SIZE)) return RV_NOSPACE;

	rv = RV_SUCCESS;
	if (size > MEM_READ_BLOCK_SIZE) {
		rv = CDH_command_new(&cmd_frame, seqnum, SS_MEMORY, SS_CMD_MM_MEMORY_READ_LONG);
		if (RV_SUCCESS == rv) {
			frame_put_u32(cmd_frame, (uint32_t)src+MEM_READ_BLOCK_SIZE);
			frame_put_u32(cmd_frame, size - MEM_READ_BLOCK_SIZE);

			vTaskDelay(nvram.mm.inter_bulk_cmd_delay_ms / portTICK_RATE_MS);
			rv = CDH_command_enqueue(cmd_frame);
		}
	}
	return rv;
}

static retval_t cmd_mem_read_chunked(const subsystem_t *self, frame_t * iframe, frame_t * oframe, uint32_t seqnum) {
	uint8_t *src=0;
	frame_t *cmd_frame;
	uint32_t offset, chunk_count, chunk_size, chunk_period;
	uint32_t i, chunk_number, chunk_offset, total_size;
	retval_t rv;

	if (RV_SUCCESS != frame_get_u32(iframe, (void*)&src)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &offset)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &chunk_count)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &chunk_size)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &chunk_period)) return RV_NOSPACE;

	total_size = chunk_size*chunk_count;

	if (RV_SUCCESS != frame_put_u32(oframe, offset)) return RV_NOSPACE;

	for (i=0; i < MEM_READ_BLOCK_SIZE && offset < total_size; i++, offset++) {
		chunk_number = offset / chunk_size;
		chunk_offset = offset % chunk_size;
		if (RV_SUCCESS != frame_put_u8(oframe, src[chunk_period*chunk_number + chunk_offset])) return RV_NOSPACE;
	}

	rv = RV_SUCCESS;
	if (offset < total_size) {
		rv = CDH_command_new(&cmd_frame, seqnum, SS_MEMORY, SS_CMD_MM_MEMORY_READ_CHUNKED);
		if (RV_SUCCESS == rv) {
			frame_put_u32(cmd_frame, (uint32_t)src);
			frame_put_u32(cmd_frame, offset);
			frame_put_u32(cmd_frame, chunk_count);
			frame_put_u32(cmd_frame, chunk_size);
			frame_put_u32(cmd_frame, chunk_period);

			vTaskDelay(nvram.mm.inter_bulk_cmd_delay_ms / portTICK_RATE_MS);
			rv = CDH_command_enqueue(cmd_frame);
		}
	}
	return rv;
}

static retval_t cmd_mem_write(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *dst=0;
	uint8_t len;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&dst)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &len)) return RV_NOSPACE;
	return frame_get_data(iframe, dst, len);
}

static retval_t cmd_mem_memcpy(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint32_t len;
	void *dst=0, *src=0;

	if (!frame_hasEnoughSpace(iframe, sizeof((uint32_t)dst)+sizeof((uint32_t)src)+sizeof(len))) {
		return RV_NOSPACE;
	}

	src = (void*)frame_get_u32_nocheck(iframe);
	dst = (void*)frame_get_u32_nocheck(iframe);
	len = frame_get_u32_nocheck(iframe);

	memcpy(dst, src, len);
    return RV_SUCCESS;
}

static retval_t cmd_mem_call(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint32_t (*addr)(uint32_t, uint32_t, uint32_t) = NULL;
	uint32_t retval, arg0 = 0, arg1 = 0, arg2 = 0;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&addr)) return RV_NOSPACE;
    (void)frame_get_u32(iframe, &arg0);
    (void)frame_get_u32(iframe, &arg1);
    (void)frame_get_u32(iframe, &arg2);

    log_report_fmt(LOG_SS_MEMORY, "calling 0x%08x(0x%08x, 0x%08x, 0x%08x)\n", addr, arg0, arg1, arg2);
    watchdog_kick(); /* give some rest in case of fw upgrade etc.. for now this just delay the check in the ISR, it doesn't really delay/disable the watchdog, so there is still an unsafe window during which a timeout can occurs. */
    FUTURE_HOOK_6(cmd_mem_call, &addr, &arg0, &arg1, &arg2, iframe, oframe);
    retval = addr(arg0, arg1, arg2);
    log_report_fmt(LOG_SS_MEMORY, "call returned 0x%08x\n", retval);
	frame_put_u32(oframe, retval);

	return RV_SUCCESS;
}

static retval_t cmd_mem_md5(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint32_t addr, size;
	MD5_CTX ctx;
	int rv;

	memset(ctx.digest, 0, sizeof(ctx.digest));
	if (frame_hasEnoughSpace(iframe, sizeof(addr)+sizeof(size))) {
		(void)frame_get_u32(iframe, &addr);
		(void)frame_get_u32(iframe, &size);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char *)addr, size);
		MD5Final(&ctx);
		log_report_fmt(LOG_SS_MEMORY, "MD5(0x%08x,0x%x): "
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				addr, size,
				ctx.digest[0], ctx.digest[1], ctx.digest[2], ctx.digest[3],
				ctx.digest[4], ctx.digest[5], ctx.digest[6], ctx.digest[7],
				ctx.digest[8], ctx.digest[9], ctx.digest[10], ctx.digest[11],
				ctx.digest[12], ctx.digest[13], ctx.digest[14], ctx.digest[15]);
		rv = RV_SUCCESS;
	} else {
		rv = RV_NOSPACE;
	}
	frame_put_data(oframe, ctx.digest, sizeof(ctx.digest));

	return rv;
}

static retval_t cmd_flash_sector_erase(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *dst=0;
	uint32_t len;
	int rv;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&dst)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &len)) return RV_NOSPACE;
    if (FLASH_ERR_OK != (rv = flash_erase(dst, len))) {
        log_report_fmt(LOG_SS_MEMORY, "flash_erase(0x%08x, %d) FAILED! (%d)\n", dst, len, rv);
    	return RV_ERROR;
    }
    log_report_fmt(LOG_SS_MEMORY_VERBOSE, "flash_erase(0x%08x, %d) success!\n", dst, len);
	return RV_SUCCESS;
}

static retval_t cmd_flash_write(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *dst=0, *data;
	uint8_t len;
	flash_err_t rv;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&dst)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &len)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_data_pointer(iframe, &data, len)) return RV_NOSPACE;

	frame_advance(iframe, len);
	rv = flash_write(dst, data, len);
    if (FLASH_ERR_OK != rv) {
        frame_put_u8(oframe, rv);
        return RV_ERROR;
    }
	return RV_SUCCESS;
}

static retval_t flash_write_direct(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *dst, *src;
	uint32_t size;
	flash_err_t err;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&dst)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&src)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&size)) return RV_NOSPACE;

	err = flash_write(dst, src, size);
	log_report_fmt(LOG_SS_MEMORY, "flash_write_direct(0x%08x, 0x%08x, %d) returned FLASH_ERR_#%d\n", dst, src, size, err);
	frame_put_u32(oframe, err);

	if (FLASH_ERR_OK != err) return RV_ERROR;
	return RV_SUCCESS;
}

static retval_t cmd_flash_sector_write_mem(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	MD5_CTX ctx;
	uint32_t len;
	void *dst=0, *src=0;
	uint8_t md5[sizeof(ctx.digest)];
	int rv;

	(void /* once is enough */)frame_get_u32(iframe, (uint32_t*)&src);
	(void /* once is enough */)frame_get_u32(iframe, (uint32_t*)&dst);
	(void /* once is enough */)frame_get_u32(iframe, &len);
	if (RV_SUCCESS != frame_get_data(iframe, &md5, sizeof(md5))) return RV_NOSPACE;

	MD5Init(&ctx);
	MD5Update(&ctx, src, len);
	MD5Final(&ctx);
	if (memcmp(ctx.digest, md5, sizeof(md5))) {
        log_report_fmt(LOG_SS_MEMORY, "invalid memory MD5: "
                "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
                ctx.digest[0], ctx.digest[1], ctx.digest[2], ctx.digest[3],
                ctx.digest[4], ctx.digest[5], ctx.digest[6], ctx.digest[7],
                ctx.digest[8], ctx.digest[9], ctx.digest[10], ctx.digest[11],
                ctx.digest[12], ctx.digest[13], ctx.digest[14], ctx.digest[15]);
        frame_put_u8(oframe, RV_ILLEGAL);
        frame_put_data(oframe, ctx.digest, sizeof(ctx.digest));
		return RV_ILLEGAL;
	}

    if (FLASH_ERR_OK != (rv = flash_erase(dst, len))) {
        log_report_fmt(LOG_SS_MEMORY, "flash_erase(0x%08x, %d) FAILED! (%d)\n", dst, len, rv);
        frame_put_u8(oframe, RV_ERROR);
    	return RV_ERROR;
    }
    log_report_fmt(LOG_SS_MEMORY_VERBOSE, "flash_erase(0x%08x, %d) success!\n", dst, len);

    if (FLASH_ERR_OK != (rv = flash_write(dst, src, len))) {
        log_report_fmt(LOG_SS_MEMORY, "_flash_write(0x%08x, 0x%08x, %d) failed! (%d)\n", dst, src, len, rv);
        frame_put_u8(oframe, RV_PARTIAL);
    	return RV_ERROR;
    }
    log_report_fmt(LOG_SS_MEMORY_VERBOSE, "flash_write_mem(0x%08x, 0x%08x, %d) success!\n", dst, src, len);
    frame_put_u8(oframe, RV_SUCCESS);
    return RV_SUCCESS;
}

#ifdef CMD_FLASH_PATCH
static retval_t cmd_flash_patch(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *dst=0, *data;
	uint8_t len, mode;
    bool is_flashpatch_bug_fixed = false;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t*)&dst)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &mode)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &len)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_data_pointer(iframe, &data, len)) return RV_NOSPACE;

	frame_advance(iframe, len);

    FUTURE_HOOK_1(flash_patch_bug_fixed, &is_flashpatch_bug_fixed);
    if (!is_flashpatch_bug_fixed) {
        return RV_NOTIMPLEMENTED;
    }

    if (FLASH_ERR_OK != flash_patch_ex(dst, data, len, mode)) return RV_ERROR;

	return RV_SUCCESS;
}
#endif

static retval_t cmd_nvram_save_all(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint32_t key;

	if (RV_SUCCESS != frame_get_u32(iframe, &key)) return RV_ILLEGAL;
	if (0x5ABEDA7A != key) return RV_ILLEGAL;

	return MEMORY_nvram_flush();
}

static retval_t cmd_nvram_save_partial(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint16_t offset;
	uint16_t size;
	uint32_t key;

	if (RV_SUCCESS != frame_get_u16(iframe, &offset)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u16(iframe, &size)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u32(iframe, &key)) return RV_NOSPACE;
	if (0x5ABEDA7A != key) return RV_ILLEGAL;

	return nvram_flush_partial(offset, size);
}

static retval_t cmd_nvram_reload(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return MEMORY_nvram_reload();
}

static retval_t cmd_nvram_read(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t *src = 0;
	uint8_t size;
	uint16_t offset;

	if (RV_SUCCESS != frame_get_u16(iframe, &offset)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &size)) return RV_NOSPACE;

	if (0 == size) size = MEM_READ_BLOCK_SIZE;
	src = ((uint8_t*)&nvram) + offset;

	if (RV_SUCCESS != frame_put_u32(oframe, (uint32_t)src)) return RV_NOSPACE;
	return frame_put_data(oframe, src, size);
}

static retval_t cmd_nvram_patch(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t *dst = 0;
	uint32_t offset;
	uint8_t size;

	if (RV_SUCCESS != frame_get_u32(iframe, &offset)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &size)) return RV_NOSPACE;

	dst = ((uint8_t*)&nvram) + offset;
	(void)frame_put_u8(oframe, size);
    (void)frame_put_data(oframe, dst, size); /* send previous data */

	return frame_get_data(iframe, dst, size);
}

static retval_t cmd_nvram_erase_all(const subsystem_t *self, frame_t *iframe, frame_t *oframe)
{
	uint32_t key;

	if (RV_SUCCESS != frame_get_u32(iframe, &key)) return RV_ILLEGAL;
	if (0x5ABEDA7A != key) return RV_PERM;

	return nvram_erase();
}

static retval_t cmd_nvram_format(const subsystem_t *self, frame_t *iframe, frame_t *oframe)
{
	uint32_t key;
	uint8_t disable_flushing;

	if (RV_SUCCESS != frame_get_u32(iframe, &key)) return RV_NOSPACE;
	if (RV_SUCCESS != frame_get_u8(iframe, &disable_flushing)) return RV_NOSPACE;

	return nvram_format_and_disable_flushing(key, disable_flushing ? true : false);
}

static retval_t cmd_mem_info(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	retval_t rv;

    frame_put_u32(oframe, MEMORY_fw_start_address());
    frame_put_u32(oframe, configTOTAL_HEAP_SIZE);
    frame_put_u32(oframe, xPortGetFreeHeapSize());
    frame_put_u32(oframe, (uintptr_t)MEMORY_uploadarea_address());
    frame_put_u32(oframe, MEMORY_uploadarea_size());
    rv = frame_put_u32(oframe, (uintptr_t)&nvram);
    rv = frame_put_u32(oframe, 0xdeadc0de); // TODO unused
    FUTURE_HOOK_3(cmd_mem_info, iframe, oframe, &rv);

    return rv;
}
const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("free:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_READ, cmd_mem_read, "read", "Reads 200 bytes from remote memory", "address:u32", "data:u8[200]"),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_READ_LONG, cmd_mem_read_long, "read", "Reads from memory <addr>, <n> bytes. The answers is broken down in many packets", "address:u32, size:u32", "address:u32,data:str"),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_READ_CHUNKED, cmd_mem_read_chunked, "readChunked", "Reads from memory <addr>, starting at <offset>, <count> chunks of <size> bytes starting every <perdiod> bytes. The answers is broken down in many packets", "address:u32,offset:u32,count:u32,size:u32,period:u32", "offset:u32,data:u8[200]"),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_WRITE, cmd_mem_write, "write", "Writes bytes to memory <addr> <len> <data>", "address:u32, data:str8", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_COPY, cmd_mem_memcpy, "copy", "Copies len bytes from src to dst <dst> <src> <len>", "from:u32, to:u32, size:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_CALL, cmd_mem_call, "call", "Calls (jumps) a specific address <dst_addr> <arg0> <arg1> <arg2>", "address:u32, arg0:u32, arg1:u32, arg2:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_MD5, cmd_mem_md5, "md5", "MD5SUM <addr> <len>", "address:u32,size:u32", "md5:u8[16]"),
    DECLARE_COMMAND(SS_CMD_MM_BITMAP, cmd_mem_bitmap, "bitmap", "Make bitmap from <start addr> <size of area> <step>", "from:u32,size:u32,step:u32", "bitmap:str"),

    DECLARE_COMMAND(SS_CMD_MM_FLASH_SECTOR_ERASE, cmd_flash_sector_erase, "flashEraseSectors", "Erases a flash sector <dst_addr> <len>", "address:u32,size:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_FLASH_WRITE, cmd_flash_write, "flashWrite", "Writes some bytes to flash, no erase is done (prefer flash_patch) <dst_addr> <len> <data>", "address:u32,data:str8", ""),
    DECLARE_COMMAND(SS_CMD_MM_FLASH_SECTOR_WRITE_FROM_MEMORY, cmd_flash_sector_write_mem, "flashWrite", "Writes some bytes to flash from memory if the md5 matches, erases sectors first. The MD5 is an array of 16 bytes #[123 1 5 3 ...]", "from:u32, to:u32, size:u32, md5:u8[16]", "rv:retval_t, md5:u8[16]"),
#ifdef CMD_FLASH_PATCH
    DECLARE_COMMAND(SS_CMD_MM_FLASH_PATCH, cmd_flash_patch, "flashPatch", "Writes some bytes to memory, saving content around in the same sector", "address:u32,mode:u8,data:str8", ""),
#endif
    DECLARE_COMMAND(SS_CMD_MM_FLASH_WRITE_FROM_MEMORY, flash_write_direct, "flashWriteDirect", "Writes some bytes to flash, <dst_addr> <src_addr> <size>", "dst:u32,src:u32,size:u32", "flash_err:u32"),

    // FIXME do we want a nvram_reset_to_default ?
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_SAVE_ALL, cmd_nvram_save_all, "nvramSaveAll", "Saves current content (full) of configuration structure in RAM to nvram <key=16r5ABEDA7A>", "key:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_SAVE_PARTIAL, cmd_nvram_save_partial, "nvramSavePartial", "Saves current content (partial) of configuration structure in RAM to nvram <key=0x5ABEDA7A>", "offset:u16, size:u16, key:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_RELOAD, cmd_nvram_reload, "nvramReload", "Reload content of nvram to configuration structure in RAM", "", ""),
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_READ, cmd_nvram_read, "nvramRead", "Read current content of configuration structure from RAM <size 0=most>", "offset:u16,size:u8", ""),
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_PATCH, cmd_nvram_patch, "nvramPatch", "Patches current content of configuration structure in RAM (not persisten)", "offset:u32,data:str8", "oldData:str8"),
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_ERASE, cmd_nvram_erase_all, "nvramErase", "Erase current configuration structure <key=16r5ABEDA7A>", "key:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_NVRAM_FORMAT, cmd_nvram_format, "nvramFormat", "Format nvram <key=16rA5A5A5A5> <disable_flushing>", "key:u32,disable_flushing:u8", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_INFO, cmd_mem_info, "info", "dump useful info", "", "startAddress:u32,heapSize:u32,freeHeap:u32,uploadAdress:u32,uploadSize:u32,nvramAddress:u32,zlibFlags:u32"),

    DECLARE_COMMAND(SS_CMD_MM_MEMORY_COMP_INIT, cmd_mem_comp_init_future, "initCompression", "Initialize compression lib <arg0> <arg1> <arg2> <arg3> (Future)", "arg0:u32, arg1:u32, arg2:u32, arg3:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_DECOMPRESS, cmd_mem_uncompress_future, "uncompress", "Uncompress from memory to memory (Future)", "srcAddress:u32, srcSize:u32, destAddress:u32, destMaxSize:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_COMPRESS, cmd_mem_compress_future, "compress", "Compress from memory to memory (Future)", "srcAddress:u32, srcSize:u32, destAddress:u32, destMaxSize:u32", ""),
#ifdef ZLIB_H
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_DECOMPRESS_Z, cmd_mem_uncompress_z, "z_uncompress", "Uncompress from memory to memory", "srcAddress:u32, srcSize:u32, destAddress:u32, destMaxSize:u32", ""),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_COMPRESS_Z, cmd_mem_compress_z, "z_compress", "Compress from memory to memory", "srcAddress:u32, srcSize:u32, destAddress:u32, destMaxSize:u32", ""),
#endif
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_COMPRESS_BCL_LZ77, cmd_mem_compress_bcl_lz, "lzCompress", "Compress from memory to memory using LZ77", "srcAddress:u32, srcSize:u32, destAddress:u32, expectedMD5:u8", "compressedSize:u32, md5:u8[16]"),
    DECLARE_COMMAND(SS_CMD_MM_MEMORY_DECOMPRESS_BCL_LZ77, cmd_mem_decompress_bcl_lz, "lzDecompress", "Decompress from memory to memory using LZ77", "srcAddress:u32, srcSize:u32, destAddress:u32, expectedSize:u32", "md5:u8[16]"),
};

static subsystem_api_t subsystem_api = {
    .main_task = &MEMORY_main_task,
    .command_execute = &ss_command_execute,
};

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_MEMORY,
    .usStackDepth = STACK_DEPTH_MEMORY,
    .id = SS_MEMORY,
    .name = "MEMORY",
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_MEMORY = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = &subsystem_state
};
