#include <canopus/frame.h>
#include <canopus/logging.h>
#include <canopus/md5.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

static DECLARE_FRAME_POOL(TheFramePool, 16, MAX_FRAME_SIZE);

inline size_t _frame_available_data(const frame_t *frame) {
	return frame->size - frame->position;
}

bool frame_hasEnoughData(const frame_t *frame, size_t wanted) {
   return _frame_available_data(frame) >= wanted ? true : false;
}

void inline frame_reset(frame_t *frame) {
    frame->position = 0;
}

void inline frame_reset_for_reading(frame_t *frame) {
	frame->size = frame->position;
	frame_reset(frame);
}

void inline frame_copy(frame_t *dst, const frame_t *src) {
	*dst = *src;
}

void inline frame_copy_for_reading(frame_t *dst, const frame_t *src) {
	frame_copy(dst, src);
	frame_reset_for_reading(dst);
}

uint8_t inline frame_get_u8_nocheck(frame_t *frame) {
   return frame->buf[frame->position++];
}

retval_t frame_get_u8(frame_t *frame, uint8_t *value) {
   if (!frame_hasEnoughData(frame, 1)) return RV_NOSPACE;
   *value = frame_get_u8_nocheck(frame);
   return RV_SUCCESS;
}

uint16_t inline frame_get_u16_nocheck(frame_t *frame) {
   uint16_t answer;
   answer  = frame->buf[frame->position++] << 8;
   answer |= frame->buf[frame->position++];
   return answer;
}

retval_t frame_get_u16(frame_t *frame, uint16_t *value) {
   if (!frame_hasEnoughData(frame, 2)) return RV_NOSPACE;
   *value = frame_get_u16_nocheck(frame);
   return RV_SUCCESS;
}

retval_t frame_get_s16(frame_t *frame, int16_t *value) {
   return frame_get_u16(frame, (uint16_t*) value);
}

uint16_t inline frame_get_u16_le_nocheck(frame_t *frame) {
   uint16_t answer;
   answer  = frame->buf[frame->position++];
   answer |= frame->buf[frame->position++] << 8;
   return answer;
}

retval_t frame_get_u16_le(frame_t *frame, uint16_t *value) {
   if (!frame_hasEnoughData(frame, 2)) return RV_NOSPACE;
   *value = frame_get_u16_le_nocheck(frame);
   return RV_SUCCESS;
}

uint32_t inline frame_get_u24_nocheck(frame_t *frame) {
   uint32_t answer;
   answer  = frame->buf[frame->position++] << 16;
   answer |= frame->buf[frame->position++] << 8;
   answer |= frame->buf[frame->position++];
   return answer;
}

retval_t frame_get_u24(frame_t *frame, uint32_t *value) {
   if (!frame_hasEnoughData(frame, 3)) return RV_NOSPACE;
   *value = frame_get_u24_nocheck(frame);
   return RV_SUCCESS;
}

uint32_t inline frame_get_u24_le_nocheck(frame_t *frame) {
   uint32_t answer;
   answer  = frame->buf[frame->position++];
   answer |= frame->buf[frame->position++] << 8;
   answer |= frame->buf[frame->position++] << 16;
   return answer;
}

retval_t frame_get_u24_le(frame_t *frame, uint32_t *value) {
   if (!frame_hasEnoughData(frame, 3)) return RV_NOSPACE;
   *value = frame_get_u24_le_nocheck(frame);
   return RV_SUCCESS;
}

uint32_t inline frame_get_u32_nocheck(frame_t *frame) {
   uint32_t answer;
   answer  = frame->buf[frame->position++] << 24;
   answer |= frame->buf[frame->position++] << 16;
   answer |= frame->buf[frame->position++] << 8;
   answer |= frame->buf[frame->position++];
   return answer;
}

retval_t frame_get_u32(frame_t *frame, uint32_t *value) {
   if (!frame_hasEnoughData(frame, 4)) return RV_NOSPACE;
   *value = frame_get_u32_nocheck(frame);
   return RV_SUCCESS;
}

uint32_t inline frame_get_u32_le_nocheck(frame_t *frame) {
   uint32_t answer;
   answer  = frame->buf[frame->position++];
   answer |= frame->buf[frame->position++] << 8;
   answer |= frame->buf[frame->position++] << 16;
   answer |= frame->buf[frame->position++] << 24;
   return answer;
}

retval_t frame_get_u32_le(frame_t *frame, uint32_t *value) {
   if (!frame_hasEnoughData(frame, 4)) return RV_NOSPACE;
   *value = frame_get_u32_le_nocheck(frame);
   return RV_SUCCESS;
}

uint64_t inline frame_get_u64_nocheck(frame_t *frame) {
   uint64_t answer;
   /* cast to avoid warning: left shift count >= width of type */
   answer  = (uint64_t)frame->buf[frame->position++] << 56;
   answer |= (uint64_t)frame->buf[frame->position++] << 48;
   answer |= (uint64_t)frame->buf[frame->position++] << 40;
   answer |= (uint64_t)frame->buf[frame->position++] << 32;
   answer |= frame->buf[frame->position++] << 24;
   answer |= frame->buf[frame->position++] << 16;
   answer |= frame->buf[frame->position++] << 8;
   answer |= frame->buf[frame->position++];
   return answer;
}

retval_t frame_get_u64(frame_t *frame, uint64_t *value) {
   if (!frame_hasEnoughData(frame, sizeof(uint64_t))) return RV_NOSPACE;
   *value = frame_get_u64_nocheck(frame);
   return RV_SUCCESS;
}

float inline frame_get_f_nocheck(frame_t *frame) {
	COMPILER_ASSERT(sizeof(float) == 4);
	float answer;

	frame_get_data_nocheck(frame, &answer, 4);
	return answer;
}

retval_t frame_get_f(frame_t *frame, float *value) {
	COMPILER_ASSERT(sizeof(float) == 4);
	return frame_get_data(frame, value, sizeof(*value));
}

void frame_get_data_nocheck(frame_t *frame, void *buf, size_t count) {
   memcpy(buf, &frame->buf[frame->position], count);
   frame->position += count;
}

retval_t frame_get_data(frame_t *frame, void *buf, size_t count) {
   if (!frame_hasEnoughData(frame, count)) return RV_NOSPACE;
   frame_get_data_nocheck(frame, buf, count);
   return RV_SUCCESS;
}

void *frame_get_data_pointer_nocheck(const frame_t *frame, size_t count) {
   if (count < 1) return NULL;
   return &frame->buf[frame->position];
}

retval_t frame_get_data_pointer(frame_t *frame, void **ptr, size_t count) {
   if (!frame_hasEnoughData(frame, count)) {
       *ptr = NULL;
       return RV_NOSPACE;
   }
   *ptr = frame_get_data_pointer_nocheck(frame, count);
   return RV_SUCCESS;
}

retval_t frame_get_data_avail(frame_t *frame, void *buf, size_t count, size_t *available) {
   retval_t answer = RV_SUCCESS;

   *available = _frame_available_data(frame);

   if (*available >= count)
       *available = count;
   else
       answer = RV_NOSPACE;

   frame_get_data_nocheck(frame, buf, *available);
   return answer;
}
/**
 * frame_transfer(dst_frame, const src_frame)
 *
 * @param[in] dst_frame
 * @param[inout] src_frame
 *
 * @return
 *
 * frame_transfer() will get data from src_frame and put it into dst_frame.
 * It reads at the current position of src_frame, moving the position forward
 * It writes at the current position of dst_Frame, moving the position forward
 */

static retval_t _frame_transfer_count(frame_t *dst_frame, frame_t *src_frame, uint32_t src_count) {
	size_t dst_space, copy_count;
	void *dst;

	dst_space = _frame_available_space(dst_frame);
	if (dst_space < src_count) copy_count = dst_space;
	else copy_count = src_count;

	dst = frame_get_data_pointer_nocheck(dst_frame, copy_count);
	frame_get_data_nocheck(src_frame, dst, copy_count);
	frame_advance(dst_frame, copy_count);

	if (dst_space < src_count) return RV_NOSPACE;
	return RV_SUCCESS;
}

retval_t frame_transfer(frame_t *dst_frame, frame_t *src_frame) {
	size_t src_count;

	src_count = _frame_available_data(src_frame);
	return _frame_transfer_count(dst_frame, src_frame, src_count);
}

retval_t frame_transfer_count(frame_t *dst_frame, frame_t *src_frame, uint32_t src_count_arg) {
	size_t src_count_real;

	src_count_real = _frame_available_data(src_frame);
	if (src_count_real > src_count_arg) src_count_real = src_count_arg;
	return _frame_transfer_count(dst_frame, src_frame, src_count_real);
}

void inline frame_advance_nocheck(frame_t *frame, size_t count) {
    frame->position += count;
}

retval_t frame_advance(frame_t *frame, int offset) {
    if (((int) frame->position + offset) < 0) return RV_ILLEGAL;
    if (offset > 0 && !frame_hasEnoughSpace(frame, offset)) return RV_NOSPACE;
    frame_advance_nocheck(frame, offset);
    return RV_SUCCESS;
}

void frame_put_u8_nocheck(frame_t *frame, uint8_t data) {
   frame->buf[frame->position++] = data;
}

retval_t frame_put_u8(frame_t *frame, uint8_t data) {
   if (!frame_hasEnoughSpace(frame, 1)) return RV_NOSPACE;
   frame->buf[frame->position++] = data;
   return RV_SUCCESS;
}

retval_t frame_put_u16(frame_t *frame, uint16_t data) {
   if (!frame_hasEnoughSpace(frame, 2)) return RV_NOSPACE;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data;
   return RV_SUCCESS;
}

retval_t frame_put_s16(frame_t *frame, int16_t data) {
   if (!frame_hasEnoughSpace(frame, 2)) return RV_NOSPACE;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data;
   return RV_SUCCESS;
}

retval_t frame_put_u16_le(frame_t *frame, uint16_t data) {
   if (!frame_hasEnoughSpace(frame, 2)) return RV_NOSPACE;
   frame->buf[frame->position++] = data;
   frame->buf[frame->position++] = data >> 8;
   return RV_SUCCESS;
}

retval_t frame_put_u24(frame_t *frame, uint32_t data) {
   if (!frame_hasEnoughSpace(frame, 3)) return RV_NOSPACE;
   frame->buf[frame->position++] = data >> 16;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data;
   return RV_SUCCESS;
}

retval_t frame_put_u24_le(frame_t *frame, uint32_t data) {
   if (!frame_hasEnoughSpace(frame, 3)) return RV_NOSPACE;
   frame->buf[frame->position++] = data;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data >> 16;
   return RV_SUCCESS;
}

retval_t frame_put_u32(frame_t *frame, uint32_t data) {
   if (!frame_hasEnoughSpace(frame, 4)) return RV_NOSPACE;
   frame->buf[frame->position++] = data >> 24;
   frame->buf[frame->position++] = data >> 16;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data;
   return RV_SUCCESS;
}

retval_t frame_put_u32_le(frame_t *frame, uint32_t data) {
   if (!frame_hasEnoughSpace(frame, 4)) return RV_NOSPACE;
   frame->buf[frame->position++] = data;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data >> 16;
   frame->buf[frame->position++] = data >> 24;
   return RV_SUCCESS;
}

retval_t frame_put_u64(frame_t *frame, uint64_t data) {
   if (!frame_hasEnoughSpace(frame, sizeof(uint64_t))) return RV_NOSPACE;
   frame->buf[frame->position++] = data >> 56;
   frame->buf[frame->position++] = data >> 48;
   frame->buf[frame->position++] = data >> 40;
   frame->buf[frame->position++] = data >> 32;
   frame->buf[frame->position++] = data >> 24;
   frame->buf[frame->position++] = data >> 16;
   frame->buf[frame->position++] = data >> 8;
   frame->buf[frame->position++] = data;
   return RV_SUCCESS;
}

retval_t frame_put_bits_by_4(frame_t *frame, uint32_t data, size_t bits) {
	uint32_t mask;
	retval_t rv;
	uint8_t value;

	if (bits % 2) return RV_ILLEGAL;

	bits -= 2;
	mask = 3 << bits;
	while (mask) {
		switch ((data >> bits) & 3) {
			case 0:
				value = 0x00;
				break;
			case 1:
				value = 0x0f;
				break;
			case 2:
				value = 0xf0;
				break;
			case 3:
				value = 0xff;
				break;
		}
		bits -= 2;
		mask >>= 2;
		rv = frame_put_u8(frame, value);
		if (RV_SUCCESS != rv) return rv;
	}
	return rv;
}

static void frame_put_hex8_nocheck(frame_t *frame, uint32_t data) {
   const uint8_t hexdigit[] = "0123456789abcdef";

   frame_put_u8(frame, hexdigit[data >> 4 & 0x0f]);
   frame_put_u8(frame, hexdigit[data & 0xF]);
}

retval_t frame_put_hex32(frame_t *frame, uint32_t data) {
   if (!frame_hasEnoughSpace(frame, 4*2)) return RV_NOSPACE;

   frame_put_hex8_nocheck(frame, (data >> 24) & 0xff);
   frame_put_hex8_nocheck(frame, (data >> 16) & 0xff);
   frame_put_hex8_nocheck(frame, (data >> 8)  & 0xff);
   frame_put_hex8_nocheck(frame,  data        & 0xff);

   return RV_SUCCESS;
}

retval_t frame_put_hex_data(frame_t *frame, const uint8_t *data, size_t count) {
	if (!frame_hasEnoughSpace(frame, count*2)) return RV_NOSPACE;

	for (;count > 0;count--,data++) {
		frame_put_hex8_nocheck(frame, *data);
	}
   return RV_SUCCESS;
}

retval_t frame_put_data(frame_t *frame, void const *buf, size_t count) {
   if (!frame_hasEnoughSpace(frame, count)) return RV_NOSPACE;
   memcpy(&frame->buf[frame->position], buf, count);
   frame->position += count;
   return RV_SUCCESS;
}

retval_t frame_put_f(frame_t *frame, float data) {
	COMPILER_ASSERT(sizeof(float) == 4);
	return frame_put_data(frame, &data, sizeof(data));
}

retval_t _frame_pool_initialize(const frame_pool_t *pool) {
	size_t i;
	uint8_t *data_buffer_base = pool->big_buffer;

	for (i=0; i < pool->count; i++) {
		pool->frames[i].flags = 0;
		pool->frames[i].buf = data_buffer_base;
		data_buffer_base += pool->buffer_size;
	}
	return RV_SUCCESS;
}

retval_t frame_pool_initialize() {
	return _frame_pool_initialize(&TheFramePool);
}

int _frame_free_count(const frame_pool_t *pool) {
	int answer = 0,i;
	taskENTER_CRITICAL();
	for (i=0; i<pool->count; i++) {
		if (FRAME_IS_AVAILABLE(&pool->frames[i])) {
			answer++;
		}
	}
	taskEXIT_CRITICAL();
	return answer;
}

int frame_free_count() {
	return _frame_free_count(&TheFramePool);
}

retval_t _frame_recycle(const frame_pool_t *pool, frame_t *frame) {
	frame->position = 0;
	frame->retry = 0;
	frame->timeout = 0;
	frame->size = pool->buffer_size;
	return RV_SUCCESS;
}

retval_t frame_recycle(frame_t *frame) {
	return _frame_recycle(&TheFramePool, frame);
}

retval_t _frame_allocate(const frame_pool_t *pool, frame_t **frame) {
	size_t i;

	taskENTER_CRITICAL();
	for (i=0; i<pool->count; i++) {
		if (FRAME_IS_AVAILABLE(&pool->frames[i])) {
			*frame = &pool->frames[i];
			(*frame)->flags = FRAME_FLAG_IN_USE;
			taskEXIT_CRITICAL();     /* As soon as possible */
			return _frame_recycle(pool, *frame);
		}
	}
	taskEXIT_CRITICAL();
	return RV_NOSPACE;
}

retval_t frame_allocate(frame_t **frame) {
	return _frame_allocate(&TheFramePool, frame);
}

retval_t _frame_allocate_retry(const frame_pool_t *pool, frame_t **frame, portTickType delay) {
	retval_t rv;

	rv = _frame_allocate(pool, frame);
	if (RV_SUCCESS != rv) {
		log_report(LOG_FRAME_VERBOSE, "Delaying in frame_allocate_retry()\n");
		vTaskDelay(delay);
		rv = _frame_allocate(pool, frame);
		if (RV_SUCCESS == rv) log_report(LOG_FRAME_VERBOSE, "Got a frame after delaying, good idea!\n");
		else log_report(LOG_FRAME, "Couldn't get a frame after delay :(\n");
	}
	return rv;
}

retval_t frame_allocate_retry(frame_t **frame, portTickType delay) {
	return _frame_allocate_retry(&TheFramePool, frame, delay);
}

void inline frame_dispose(frame_t *frame) {
	frame->flags = 0;
}

void frame_compute_mac(const frame_t *frame, uint32_t nonce, const uint8_t *key, size_t key_length, frame_mac_t *mac) {
	MD5_CTX ctx;

	uint8_t *data;
	size_t data_length;
	uint8_t nonce_buf[4];

	nonce_buf[0] = nonce & 0xFF;
	nonce_buf[1] = (nonce >> 8) & 0xFF;
	nonce_buf[2] = (nonce >> 16) & 0xFF;
	nonce_buf[3] = (nonce >> 24) & 0xFF;

	data_length = _frame_available_data(frame);
	data = frame_get_data_pointer_nocheck(frame, data_length);

	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)key, key_length);
	MD5Update(&ctx, (unsigned char*)&nonce_buf, sizeof(nonce_buf));
	if (NULL != data) MD5Update(&ctx, data, data_length);
	MD5Final(&ctx);

	*mac = ctx.digest[0] + (ctx.digest[1] << 8) + (ctx.digest[2] << 16);
}

retval_t frame_verify_mac(const frame_t *frame, uint32_t nonce, const uint8_t *key, size_t key_length, frame_mac_t expected_mac) {
	frame_mac_t real_mac;

	frame_compute_mac(frame, nonce, key, key_length, &real_mac);
	if (expected_mac == real_mac) return RV_SUCCESS; /* uint32_t comparison */
	return RV_ERROR;
}
