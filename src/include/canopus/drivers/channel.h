#ifndef _CANOPUS_CHANNEL_H_
#define _CANOPUS_CHANNEL_H_

#include <canopus/types.h>
#include <canopus/frame.h>

#include <FreeRTOS.h>
#include <semphr.h>

typedef struct channel_t channel_t;
typedef /*TODO const*/ struct channel_driver_t channel_driver_t;
typedef const struct channel_driver_config_t channel_driver_config_t;

/* Channels public API:
 *  This are the functions used to communicate via a channel,
 *  This is what most developers will use, once the channels are defined and configured
 */

/**
 * @return RV_TIMEOUT RV_SUCCESS
 */
retval_t channel_open(const channel_t *const chan);
retval_t channel_close(const channel_t *const chan);
retval_t channel_send(const channel_t *const chan, frame_t *const send_frame);
retval_t channel_recv(const channel_t *const chan, frame_t *const recv_frame);
retval_t channel_transact(const channel_t *const chan, frame_t *const send_frame,
                          uint32_t delay_ms, frame_t *const recv_frame);
retval_t channel_lock(const channel_t *const chan, portTickType xBlockTime);
retval_t channel_unlock(const channel_t *const chan);

// FIXME used? lead to error on TMS570
#define DECLARE_CHANNEL(__driver, __config, __state_type)	\
	(channel_t){											\
		.config = (const channel_config_t *)(__config),		\
		.state  = (channel_state_t *)&(__state_type){},	\
		.driver = __driver,								\
	}

#define DECLARE_CHANNEL_CONFIG(__flags, __lock_timeout_ms, __transaction_timeout_ms)	\
					{										\
		.flags = __flags,									\
		.lock_timeout_ms = __lock_timeout_ms,				\
		.transaction_timeout_ms = __transaction_timeout_ms  \
}
/**
 * Channel.
 *
 * High level abstraction of basic IO operations over a communication channel.
 */

#define CHANNEL_MAX_LOCK_TIMEOUT	((1<<24)-1)
#define CHANNEL_FLAG_NO_AUTO_LOCK	(1<<0)
#define CHANNEL_FLAG_OTHER_1		(1<<5)
#define CHANNEL_FLAG_OTHER_2		(1<<6)
#define CHANNEL_FLAG_OTHER_3		(1<<7)
#define CHANNEL_FLAG_LAST_POSSIBLE	(1<<7)

typedef const struct channel_config_t {
	unsigned flags:8;
	unsigned lock_timeout_ms:24;
	uint32_t transaction_timeout_ms;
} channel_config_t;

typedef struct channel_state_t  {
	bool is_open;
	xSemaphoreHandle send_mutex;
	xSemaphoreHandle recv_mutex;
} channel_state_t;

struct channel_t {
    const channel_config_t * const config;
    channel_state_t        * const state;
    const channel_driver_t * const driver;
};

/**
 * Channel Driver Operations.
 *
 * These operations are to be implemented by Channel Drivers to
 * provide concrete implementations for the higher-level Channels layer.
 *
 * Refer to the higher-level Channel Public API for more information
 * on the expected behavior for these operations.
 */

/* To be called from `channel_driver_initialize` */
typedef retval_t driver_initialize_t(
        const channel_driver_t * const driver);

/* To be called from `channel_driver_deinitialize` */
typedef retval_t driver_deinitialize_t(
        const channel_driver_t * const driver);

/* IO: Opens `channel` for later IO interaction */
typedef retval_t channel_open_t(
		const channel_t * const channel);

/* IO: Closes `channel`, preventing further IO interaction */
typedef retval_t channel_close_t(
		const channel_t * const channel);

/* IO: Send data from `send_frame` to `channel` */
typedef retval_t channel_send_t(
		const channel_t * const channel,
        frame_t * const send_frame,
        const size_t count);

/* IO:  Recieve into `send_frame` data from `channel` */
typedef retval_t channel_recv_t(
        const channel_t * const channel,
        frame_t * const recv_frame,
        const size_t count);

/* IO: Send data from `send_frame` to `channel`, and then receive into
 * `send_frame` data from `channel` */
typedef retval_t channel_transact_t(
        const channel_t * const channel,
        frame_t * const send_frame,
        uint32_t delay_ms,
        frame_t * const recv_frame,
        const size_t send_bytes,
        const size_t recv_bytes);

/* Channel Drivers:
 * 	The channel driver implements all necessary to operate a channel.
 * 	There may be a single channel driver for many channels of the same type,
 * 	for example, a single SPI channel driver, and one SPI channel per device.
 */

#define DECLARE_CHANNEL_DRIVER(__api, __config, __state_type)			\
		(channel_driver_t) {											\
			.config = (channel_driver_config_t*)(__config),				\
			.state = (channel_driver_state_t *)&(__state_type){},		\
			.api  = (const channel_driver_api_t *)(__api),				\
		};

struct channel_driver_config_t { };

typedef struct channel_driver_state_t {
	bool is_initialized;
} channel_driver_state_t;

typedef const struct channel_driver_api_t channel_driver_api_t;

struct channel_driver_t {
    channel_driver_config_t		*const config;
    channel_driver_state_t		* state;
    channel_driver_api_t		*const api;
};

/**
 * Channel Driver implementation.
 *
 * Common operations for a channel Driver, as well as initialization an
 * deinitialization routines are kept in this structure to be shared among
 * different instances of the same channel driver.
 */
struct channel_driver_api_t {
	driver_initialize_t *initialize;
	driver_deinitialize_t *deinitialize;
	channel_open_t *open;
	channel_close_t *close;
	channel_send_t *send;
	channel_recv_t *recv;
	channel_transact_t *transact;
};

retval_t channel_driver_initialize(
        const channel_driver_t        * const driver);

retval_t channel_driver_deinitialize(
        const channel_driver_t * const driver);
#endif
