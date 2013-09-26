/* a channel link that lives inside a buffer - analogous to python's stringio
 * ONLY TO BE USED FOR TESTING - NOT INTENDED FOR PRODUCTION
 *
 * auteur: manuel
 */

#ifndef _MEMORY_CHANNEL_LINK_DRIVER_H_
#define _MEMORY_CHANNEL_LINK_DRIVER_H_

#include <canopus/frame.h>
#include <canopus/drivers/channel.h>

#include <FreeRTOS.h>
#include <semphr.h>

/**
 * Channel link
 **/

typedef struct {
    channel_config_t common;
    frame_t * recv_frame;
    frame_t * send_frame;
} memory_channel_config_t;

typedef struct {
    channel_state_t common;
    frame_t * recv_frame;
    frame_t * send_frame;
    bool is_open; // TODO remove (duplicate common.is_open */
    int read_pos;
    int available_bytes;
    xSemaphoreHandle send_mutex;
    xSemaphoreHandle recv_mutex;
} memory_channel_state_t;

/**
 * Channel link driver
 **/

extern const channel_driver_t memory_channel_driver;

typedef struct {
    channel_driver_config_t common;
} memory_channel_driver_config_t;

typedef struct {
    channel_driver_state_t common;
} memory_channel_driver_state_t;

#define DECLARE_CHANNEL_MEMORY_FRAMES(_send_frame, _recv_frame)				   			\
	(channel_t){														   			\
			.config = (const channel_config_t *)&(const memory_channel_config_t){	\
				.send_frame = _send_frame,								   				\
				.recv_frame = _recv_frame,								   				\
			},															   				\
		.state  = (channel_state_t *)&(memory_channel_state_t){},					\
		.driver = &memory_channel_driver,										\
	}

#define DEFINE_CHANNEL_MEMORY_FRAMES(_name, _send_frame, _recv_frame) \
	static const memory_channel_config_t _name##_config = { \
		.send_frame = _send_frame, \
		.recv_frame = _recv_frame, \
	}; \
	/*const*/ channel_t _name = { \
		.config = (channel_config_t const*)&_name##_config, \
		.state  = (channel_state_t *)&(memory_channel_state_t){}, \
		.driver = &memory_channel_driver, \
	}

#define DECLARE_CHANNEL_MEMORY_SIZE(_size) \
    DECLARE_CHANNEL_MEMORY_FRAMES( \
        &(frame_t)DECLARE_FRAME_SPACE(_size), \
        &(frame_t)DECLARE_FRAME_SPACE(_size))

retval_t memory_channel_write_for_recv(const channel_t * const link, frame_t * frame);
frame_t *memory_channel_get_send_frame(const channel_t * const link);

#endif
