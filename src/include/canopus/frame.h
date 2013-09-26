#ifndef _CANOPUS_FRAME_H_
#define _CANOPUS_FRAME_H_

/*
 * A frame is a mean to transport data from one place to another,
 * espcially to-from devices and to-from another computer. There
 * are usually to distinct users of a single frame:
 *  . The source/producer, who has data to share. Creates a frame, puts the
 *    data in the frame, and gives the frame to someone else
 *  . The destination/consumer, who gets a frame with data in it, takes
 *    the data out and knows how to interpret it.
 *
 * A frame has a very precise life cycle:
 *  . It's born (created)
 *  . Data is put into it
 *  . It's mutated so someone else can read the data
 *  . It's discarded, or reused only to read the same data again.
 *
 * There are basically two ways to create a frame
 *  . A static frame can be created at compile time with data
 *    already in it, using the DECLARE_FRAME() macro family.
 *  . A frame can be created with no data (but storage space). In
 *    this case data will be added using the frame_put_*() function
 *    family. And then, when it's ready, reset_for_reading() will
 *    set the length to accomodate up to the last writen bit, and
 *    reset the frame so the destination can read the data.
 *  . In either case, after reading all or some of it, the frame can
 *    be reset (with frame_reset()) to read again the same data. This
 *    is usefull in static frames, so fixed frames can be stored in
 *    read only memory.
 *
 * SEE test_frame.c for documentation on how to use them
 */

#include <canopus/types.h>
#include <canopus/time.h>

#include <FreeRTOS.h>

#define FRAME_FLAG_IN_USE		(1 << 0)
#define FRAME_FLAG_IS_TRUSTED	(1 << 1)

#define FRAME_IS_IN_USE(__frame)		((__frame)->flags & FRAME_FLAG_IN_USE)
#define FRAME_IS_AVAILABLE(__frame)		(!FRAME_IS_IN_USE(__frame))
#define FRAME_IS_TRUSTED(__frame)		((__frame)->flags & FRAME_FLAG_IS_TRUSTED)

typedef struct frame_t {
	uint8_t *buf;
	size_t size;
	size_t position;
	uint8_t flags;
	timeout_t timeout; // FIXME rename timeout_ms? or use portTickType ticks instead?

	/* FIXME: retry is not currently used in the Channel API as it is today */
	uint32_t retry;
} frame_t;

#define DECLARE_FRAME_SIZE_TIMEOUT_RETRY(__buf, __size, __timeout, __retry) {    \
    .buf = (uint8_t *)(__buf),                              \
    .size = (__size),                                       \
    .position = 0,                                          \
    .flags = 0,                                             \
    .timeout = (__timeout),                                 \
    .retry = (__retry)                                      \
}

#define DECLARE_FRAME_TIMEOUT_RETRY(__buf, __timeout, __retry) \
	DECLARE_FRAME_SIZE_TIMEOUT_RETRY(__buf, sizeof __buf, __timeout, 0)
#define DECLARE_FRAME_TIMEOUT(__buf, __timeout) \
	DECLARE_FRAME_TIMEOUT_RETRY(__buf, __timeout, 0)
#define DECLARE_FRAME(__buf) DECLARE_FRAME_TIMEOUT(__buf, 0)
#define DECLARE_FRAME_SPACE(__buffer_size)			\
	DECLARE_FRAME_TIMEOUT((uint8_t[__buffer_size]){}, 0)
#define DECLARE_FRAME_BYTES(first, ...)	\
	DECLARE_FRAME(((uint8_t[]){first,__VA_ARGS__}))
#define DECLARE_FRAME_SIZE(__buf, __size) \
		DECLARE_FRAME_SIZE_TIMEOUT_RETRY(__buf, __size, 0, 0)
#define DECLARE_FRAME_NONUL(__buf) \
	DECLARE_FRAME_SIZE(__buf, sizeof(__buf)-1)
/**
 * @retval true if frame has 'wanted' bytes available, false else.
 */
bool frame_hasEnoughData(const frame_t *frame, size_t wanted);
size_t _frame_available_data(const frame_t *frame);
void frame_reset(frame_t *frame);
void frame_reset_for_reading(frame_t *frame);
void frame_copy(frame_t *dst, const frame_t *src);
void frame_copy_for_reading(frame_t *dst, const frame_t *src);
#define frame_hasEnoughSpace frame_hasEnoughData
#define _frame_available_space _frame_available_data

/** frame_get_* family checks if there's enough available data
 * @retval RV_SUCCESS, RV_NOSPACE
 */
retval_t frame_get_u8(frame_t *frame, uint8_t *value);
retval_t frame_get_u16(frame_t *frame, uint16_t *value);
retval_t frame_get_s16(frame_t *frame, int16_t *value);
retval_t frame_get_u16_le(frame_t *frame, uint16_t *value);
retval_t frame_get_u24(frame_t *frame, uint32_t *value);
retval_t frame_get_u24_le(frame_t *frame, uint32_t *value);
retval_t frame_get_u32(frame_t *frame, uint32_t *value);
retval_t frame_get_u32_le(frame_t *frame, uint32_t *value);
retval_t frame_get_u64(frame_t *frame, uint64_t *value);
retval_t frame_get_f(frame_t *frame, float *value);
retval_t frame_get_data(frame_t *frame, void *buf, size_t count);
retval_t frame_get_data_pointer(frame_t *frame, void **ptr, size_t count);
retval_t frame_get_data_avail(frame_t *frame, void *buf, size_t count, size_t *available);
retval_t frame_transfer(frame_t *dst_frame, frame_t *src_frame);
retval_t frame_transfer_count(frame_t *dst_frame, frame_t *src_frame, uint32_t src_count);
retval_t frame_advance(frame_t *frame, int offset);

/* frame_get_*_nocheck() family makes no checks on the available data */
uint8_t frame_get_u8_nocheck(frame_t *frame);
uint16_t frame_get_u16_nocheck(frame_t *frame);
uint16_t frame_get_u16_le_nocheck(frame_t *frame);
uint32_t frame_get_u24_nocheck(frame_t *frame);
uint32_t frame_get_u24_le_nocheck(frame_t *frame);
uint32_t frame_get_u32_nocheck(frame_t *frame);
uint32_t frame_get_u32_le_nocheck(frame_t *frame);
uint64_t frame_get_u64_nocheck(frame_t *frame);
void frame_get_data_nocheck(frame_t *frame, void *buf, size_t count);
void *frame_get_data_pointer_nocheck(const frame_t *frame, size_t count);
void frame_advance_nocheck(frame_t *frame, size_t count);

/** frame_put_* family
 * @retval RV_SUCCESS, RV_NOSPACE
 */
retval_t frame_put_u8(frame_t *frame, uint8_t data);
retval_t frame_put_u16(frame_t *frame, uint16_t data);
retval_t frame_put_s16(frame_t *frame, int16_t data);
retval_t frame_put_u16_le(frame_t *frame, uint16_t data);
retval_t frame_put_u24(frame_t *frame, uint32_t data);
retval_t frame_put_u24_le(frame_t *frame, uint32_t data);
retval_t frame_put_u32(frame_t *frame, uint32_t data);
retval_t frame_put_u32_le(frame_t *frame, uint32_t data);
retval_t frame_put_u64(frame_t *frame, uint64_t data);
retval_t frame_put_data(frame_t *frame, void const *data, size_t count);
retval_t frame_put_f(frame_t *frame, float data);
retval_t frame_put_hex32(frame_t *frame, uint32_t data);
retval_t frame_put_hex_data(frame_t *frame, const uint8_t *data, size_t count);
retval_t frame_put_bits_by_4(frame_t *frame, uint32_t data, size_t bits);

void frame_put_u8_nocheck(frame_t *frame, uint8_t data);

/** Frame pools **/
typedef struct frame_pool_t {
	size_t count;
	size_t buffer_size;
	frame_t *frames;
	uint8_t *big_buffer;
} frame_pool_t;

#define DECLARE_FRAME_POOL(__name, __count, __each_buffer_size)			\
	frame_t __name##_frames[__count];							\
	uint8_t __name##_big_buffer[__count * __each_buffer_size];	\
	frame_pool_t __name  = {									\
		.count = __count,										\
		.buffer_size = __each_buffer_size,						\
		.frames = __name##_frames,								\
		.big_buffer = __name##_big_buffer}

#define MAX_FRAME_SIZE			300

retval_t frame_pool_initialize();
int frame_free_count();
retval_t frame_recycle(frame_t *frame);
retval_t frame_allocate(frame_t **frame);
retval_t frame_allocate_retry(frame_t **frame, portTickType delay);
void frame_dispose(frame_t *frame);


typedef uint32_t frame_mac_t;

void frame_compute_mac(const frame_t *frame, uint32_t nonce, const uint8_t *key, size_t key_length, frame_mac_t *mac);
retval_t frame_verify_mac(const frame_t *frame, uint32_t nonce, const uint8_t *key, size_t key_length, frame_mac_t expected_mac);

#endif
