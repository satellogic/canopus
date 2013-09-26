#ifndef _CANOPUS_TMS570_I2C_DRIVER_H
#define _CANOPUS_TMS570_I2C_DRIVER_H

#include <canopus/drivers/channel.h>

#include <i2c.h>

extern const channel_driver_t tms570_i2c_channel_driver;

typedef const struct tms570_i2c_channel_driver_config_st {
	channel_driver_config_t common;
} tms570_i2c_channel_driver_config_t;

typedef struct tms570_i2c_channel_driver_state_st {
	channel_driver_state_t common;
} tms570_i2c_channel_driver_state_t;

// ----

typedef struct tms570_i2c_channel_config_t {
	channel_config_t common;
	i2cBASE_t *base;
    uint32_t slave_address;
    uint32_t baudrate_khz;
} tms570_i2c_channel_config_t;

typedef struct tms570_i2c_channel_state_t {
	channel_state_t common;
	bool debug;
} tms570_i2c_channel_state_t;

#define DEFINE_CHANNEL_I2C(_name, _slave_addr, _baudrate, _timeout_ms)		\
	static const tms570_i2c_channel_config_t _name##_config = {\
					.common.transaction_timeout_ms = _timeout_ms, \
					.base = i2cREG1,													\
					.slave_address = _slave_addr,										\
					.baudrate_khz = _baudrate,												\
				};\
	const channel_t _name = {															\
		.config = (channel_config_t const*)&_name##_config,														\
		.state  = (channel_state_t *)&(tms570_i2c_channel_state_t){},			\
		.driver = &tms570_i2c_channel_driver,									\
	}

#endif
