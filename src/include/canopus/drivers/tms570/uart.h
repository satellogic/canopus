#ifndef _CANOPUS_TMS570_UART_DRIVER_H
#define _CANOPUS_TMS570_UART_DRIVER_H

#include <canopus/drivers/channel.h>

#include <sci.h>

#define TMS570_SCI_MAX 2

extern const channel_driver_t tms570_sci_channel_driver;

typedef const struct tms570_sci_channel_driver_config_st {
	channel_driver_config_t common;
} tms570_sci_channel_driver_config_t;

typedef struct tms570_sci_channel_driver_state_st {
	channel_driver_state_t common;
} tms570_sci_channel_driver_state_t;

// ----

typedef struct tms570_sci_channel_config_t {
	channel_config_t common;
	sciBASE_t *base;
	uint32_t interrupt_bits; /* SCI_TX_INT | SCI_RX_INT */
    uint32_t baudrate;
} tms570_sci_channel_config_t;

typedef struct tms570_sci_channel_state_t {
	channel_state_t common;
	uint32_t id;
	portTickType txBlockTimePerByte;
} tms570_sci_channel_state_t;

#define DECLARE_CHANNEL_UART(_name, _base, _bits, _baudrate, _timeout_ms)		\
	static const tms570_sci_channel_config_t _name##_config = {\
					.common.transaction_timeout_ms = _timeout_ms, \
					.base = _base,														\
					.interrupt_bits = _bits,											\
					.baudrate = _baudrate,												\
				};\
	const channel_t _name = {															\
		.config = (channel_config_t const*)&_name##_config,														\
		.state  = (channel_state_t *)&(tms570_sci_channel_state_t){},			\
		.driver = &tms570_sci_channel_driver,									\
	}

#endif
