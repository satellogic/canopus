#ifndef _CANOPUS_TMS570_SPI_DRIVER_H
#define _CANOPUS_TMS570_SPI_DRIVER_H

#include <canopus/drivers/channel.h>

#include <spi.h>

extern const channel_driver_t tms570_spi_channel_driver;

typedef const struct tms570_spi_channel_driver_config_st {
	channel_driver_config_t common;
} tms570_spi_channel_driver_config_t;

typedef struct tms570_spi_channel_driver_state_st {
	channel_driver_state_t common;
} tms570_spi_channel_driver_state_t;

// ----

typedef struct tms570_spi_channel_config_t {
	channel_config_t common;
	spiBASE_t *base;

	// TODO bitrate?

	SPIDATAFMT_t DFSEL;					// [per CS]
	uint8_t wordsize_in_bits;

	uint8_t cs;
	uint8_t cs_hold_enabled;			// [per CS]

	uint8_t cs_hold_delay;				// T2CDELAY [per module]
	uint8_t cs_setuptime_delay;			// C2TDELAY [per module]

	uint8_t transactions_wait_delay;	// WDELAY [per transfer]
} tms570_spi_channel_config_t;

typedef struct tms570_spi_channel_state_t {
	channel_state_t common;
	uint32_t id;
	spiDAT1_t dat;
} tms570_spi_channel_state_t;

#define DEFINE_CHANNEL_SPI(_name, _base, _cs, _wbits, _fmt, _cshold, _wdel, _timeout_ms) \
	static const tms570_spi_channel_config_t _name##_config = {\
					.common = DECLARE_CHANNEL_CONFIG(0, _timeout_ms, _timeout_ms),		\
					.base = _base,														\
					.DFSEL = _fmt,												\
					.wordsize_in_bits = _wbits,									\
					.cs = _cs,											\
					.cs_hold_enabled = _cshold,											\
					.transactions_wait_delay = _wdel,							\
				};\
	const channel_t _name = {															\
		.config = (channel_config_t const*)&_name##_config,														\
		.state  = (channel_state_t *)&(tms570_spi_channel_state_t){},			\
		.driver = &tms570_spi_channel_driver,									\
	}

#define DECLARE_CHANNEL_SPI(_base, _cs, _wbits, _fmt, _cshold, _wdel, _timeout_ms) 		\
	(const channel_t){																	\
			.config = (const channel_config_t *)&(const tms570_spi_channel_config_t) {	\
				.common.transaction_timeout_ms = _timeout_ms, 							\
				.base = _base,															\
				.DFSEL = _fmt,															\
				.wordsize_in_bits = _wbits,												\
				.cs = _cs,																\
				.cs_hold_enabled = _cshold,												\
				.transactions_wait_delay = _wdel,										\
			},																			\
			.state  = (channel_state_t *)&(tms570_spi_channel_state_t){},				\
			.driver = &tms570_spi_channel_driver,										\
		}

#endif

void spi1Init();
void spi3Init();
