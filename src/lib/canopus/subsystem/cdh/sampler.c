#include <canopus/assert.h>
#include <canopus/drivers/channel.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/cdh.h>
#include <canopus/subsystem/mm.h>
#include <canopus/drivers/flash.h>

extern const char _cdh_sample_addr;

struct {
	union {
		uint16_t offset;
		void *address;
	} source;
	uint16_t source_size;
	uint16_t samples_wanted;
	uint16_t interval;
	portTickType last_sampled_ticks;
	enum {
		CDH_SAMPLE_TYPE_BEACON  = 0 << 0,
		CDH_SAMPLE_TYPE_MEMORY  = 1 << 0,
		CDH_SAMPLE_TYPE_MASK	= 1 << 0,
		CDH_SAMPLE_FLASH   		= 1 << 1,
	} flags;
	frame_t data;
} cdh_sample_config;

void cdh_sample_init() {
	cdh_sample_config.flags = 0;
	cdh_sample_config.source_size  = 0;
	cdh_sample_config.samples_wanted = 0;

    assert(MEMORY_uploadarea_size() > 0);
    cdh_sample_config.data = (frame_t)DECLARE_FRAME_SIZE_TIMEOUT_RETRY(MEMORY_uploadarea_address(), MEMORY_uploadarea_size(), 0, 0);
}

#define ELAPSED_SECONDS_FROM_TO(start, stop)	((stop-start) * portTICK_RATE_MS / 1000)

static retval_t cdh_sample_data(void *source, uint16_t size) {
	bool this_wanted;
	portTickType now;
	retval_t rv;
	uint16_t flashW_size = 0;

	now = xTaskGetTickCount();
	this_wanted = ELAPSED_SECONDS_FROM_TO(cdh_sample_config.last_sampled_ticks, now) > cdh_sample_config.interval;

	if (!this_wanted) return RV_NACK;

	cdh_sample_config.last_sampled_ticks = now;

	rv = frame_put_data(&cdh_sample_config.data, source, size);
	cdh_sample_config.samples_wanted--;

	if (0 == cdh_sample_config.samples_wanted) {
		// We are done, save to flash if required
		if (cdh_sample_config.flags & CDH_SAMPLE_FLASH) {
			cdh_sample_config.samples_wanted = 0;

			flashW_size = cdh_sample_config.data.position;
			if (flashW_size > 8*1024) flashW_size = 8*1024; /* 8K @ .cdh_sample in flash1 */

			frame_reset(&cdh_sample_config.data);

			rv = flash_erase(&_cdh_sample_addr, flashW_size);
			FUTURE_HOOK_3(cdh_sample_flash_erase_now, &_cdh_sample_addr, &flashW_size, &rv);

			rv = flash_write(&_cdh_sample_addr, frame_get_data_pointer_nocheck(&cdh_sample_config.data, flashW_size), flashW_size);

			FUTURE_HOOK_2(cdh_sample_flash_write_now, &cdh_sample_config.data, &rv);
		}
	}

	return rv;
}

retval_t cdh_sample_beacon_tick(frame_t *beacon) {
	uint8_t *source;

	if (0 == cdh_sample_config.samples_wanted) return RV_NACK;

	if (CDH_SAMPLE_TYPE_BEACON != (cdh_sample_config.flags & CDH_SAMPLE_TYPE_MASK)) return RV_NACK;

	if (!frame_hasEnoughData(beacon, cdh_sample_config.source.offset + cdh_sample_config.source_size)) {
		return RV_ILLEGAL;
	}

	source  = frame_get_data_pointer_nocheck(beacon, cdh_sample_config.source_size);
	source += cdh_sample_config.source.offset;

	return cdh_sample_data(source, cdh_sample_config.source_size);
}

retval_t cdh_sample_memory_tick() {
	if (0 == cdh_sample_config.samples_wanted) return RV_NACK;

	if (CDH_SAMPLE_TYPE_MEMORY != (cdh_sample_config.flags & CDH_SAMPLE_TYPE_MASK)) return RV_NACK;

	return cdh_sample_data(cdh_sample_config.source.address, cdh_sample_config.source_size);
}

#undef ELAPSED_SECONDS_FROM_TO

retval_t cmd_sample_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint16_t offset;
	uint16_t source_size;
	uint16_t interval;
	uint16_t count;

	if (RV_SUCCESS != frame_get_u16(iframe, &offset)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &source_size)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &count)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &interval)) return RV_ILLEGAL;

	// First turn off, reconfigure, then turn on
	cdh_sample_config.samples_wanted   = 0;
	cdh_sample_config.interval 		   = interval;
	cdh_sample_config.last_sampled_ticks = 0;
	cdh_sample_config.source_size      = source_size;
	cdh_sample_config.source.offset    = offset;
	cdh_sample_config.flags			   = CDH_SAMPLE_TYPE_BEACON;

	frame_reset(&cdh_sample_config.data);

	cdh_sample_config.samples_wanted = count;
	return RV_SUCCESS;
}

retval_t cmd_sample_memory(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	void *source_addr;
	uint16_t source_size;
	uint16_t interval;
	uint16_t count;

	if (RV_SUCCESS != frame_get_u32(iframe, (uint32_t *)&source_addr)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &source_size)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &count)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &interval)) return RV_ILLEGAL;

	// First turn off, reconfigure, then turn on
	cdh_sample_config.samples_wanted   = 0;
	cdh_sample_config.interval 		   = interval;
	cdh_sample_config.last_sampled_ticks = 0;
	cdh_sample_config.source_size      = source_size;
	cdh_sample_config.source.address   = source_addr;
	cdh_sample_config.flags			   = CDH_SAMPLE_TYPE_MEMORY;

	frame_reset(&cdh_sample_config.data);

	cdh_sample_config.samples_wanted = count;
	return RV_SUCCESS;
}

retval_t cmd_sample_flash_when_done(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t flash_when_done;

	if (RV_SUCCESS != frame_get_u8(iframe, &flash_when_done)) return RV_ILLEGAL;

	if (flash_when_done) cdh_sample_config.flags |= CDH_SAMPLE_FLASH;
	else cdh_sample_config.flags &= ~CDH_SAMPLE_FLASH;
	return RV_SUCCESS;
}

retval_t cmd_sample_retrieve(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_t data_copy;

	frame_copy(&data_copy, &cdh_sample_config.data);
	frame_reset_for_reading(&data_copy);
	frame_put_u16(oframe, cdh_sample_config.samples_wanted);
	frame_put_u16(oframe, cdh_sample_config.data.position);
	return frame_transfer(oframe, &data_copy);
}


