#include <canopus/types.h>
#include <canopus/logging.h>

#include <canopus/subsystem/subsystem.h>

#include "string.h"

#define BITMAP_MARK 0xffffff

retval_t
cmd_mem_bitmap(const subsystem_t *self, frame_t *iframe, frame_t *oframe)
{
	uint8_t *data_ptr, found = 0, mark = 0;
	uint32_t step_size, idx = 0, data_index = 0, data_size;

	if (!frame_hasEnoughSpace(iframe, sizeof(data_ptr)+sizeof(data_size)+sizeof(step_size))) {
		log_report(LOG_ALL, "BITMAP: no space\n");
		return RV_NOSPACE;
	}
	(void)frame_get_u32(iframe, (uint32_t *)&data_ptr);
	(void)frame_get_u32(iframe, &data_size);
	(void)frame_get_u32(iframe, &step_size);

	log_report_fmt(LOG_SS_MEMORY, "BITMAP addr:0x%08x size:%d step:%d\r\n",
			(uint32_t)data_ptr, data_size, step_size);

	while (data_index < data_size && _frame_available_space(oframe)) {
		int i, s = step_size, is_erased = 1;
		if (data_size < step_size) s = data_size;

		for (i = 0; i < s; i++) {
			if (data_ptr[data_index + i] != 0xff) {
				is_erased = 0;
				break;
			}
		}
		log_report_fmt(LOG_SS_MEMORY_VERBOSE, "BITMAP index:%d erased=%d space=%d sz=%d/%d\r\n",
				idx, is_erased, _frame_available_space(oframe), data_index, data_size);

		if (is_erased) {
			found = 1;
			if (idx < 0x100) {
				frame_put_u8(oframe, idx);
			} else {
				if (!mark) {
					frame_put_u24(oframe, BITMAP_MARK);
					mark = 1;
				}
				frame_put_u16(oframe, idx);
			}
		}
		data_index += s;
		idx++;
	}
	if (!found) {
		frame_put_u24(oframe, BITMAP_MARK);
	}

	return RV_SUCCESS;
}
