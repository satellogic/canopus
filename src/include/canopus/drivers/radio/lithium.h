/* constants for Lithium commands 
   taken from Radio_Interface_Manual_04012012.pdf
   (http://www.astrodev.com/public_html2/downloads/firmware/Li1_Programming_Pack_R3pt01.zip)
*/
#ifndef _DRIVERS_LITHIUM_H
#define _DRIVERS_LITHIUM_H

#include <canopus/types.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/radio/ax25.h>

#define LITHIUM1_MTU_SIZE 255 /* FIXME I suppose but don't have the specs. */

/* Commands */
typedef enum lithium_cmd_e {
    LITHIUM_CMD_NO_OP                   = 0x01,
    LITHIUM_CMD_RESET_SYSTEM            = 0x02,
    LITHIUM_CMD_TRANSMIT_DATA           = 0x03,
    LITHIUM_CMD_RECEIVE_DATA            = 0x04,
    LITHIUM_CMD_GET_TRANSCEIVER_CONFIG  = 0x05,
    LITHIUM_CMD_SET_TRANSCEIVER_CONFIG  = 0x06,
    LITHIUM_CMD_TELEMETRY_QUERY         = 0x07,
    LITHIUM_CMD_WRITE_FLASH             = 0x08,
    LITHIUM_CMD_RF_CONFIG               = 0x09,
    LITHIUM_CMD_BEACON_DATA             = 0x10,
    LITHIUM_CMD_BEACON_CONFIG           = 0x11,
    LITHIUM_CMD_READ_FIRMWARE_REVISION  = 0x12,
    LITHIUM_CMD_WRITE_OVER_AIR_KEY      = 0x13,
    LITHIUM_CMD_FIRMWARE_UPDATE         = 0x14,
    LITHIUM_CMD_FIRMWARE_PACKET         = 0x15,
    LITHIUM_CMD_FAST_PA_SET             = 0x20,
    LITHIUM_MAX_CMD                     /*= LITHIUM_CMD_FAST_PA_SET+1*/
} lithium_cmd_t;

#define LITHIUM_VALID_COMMAND(cmd) ((cmd >= 0x01 && cmd <= 0x15) || cmd == 0x20)

/* Command "direction" */
#define LITHIUM_DIRECTION_INPUT             0x10
#define LITHIUM_DIRECTION_OUTPUT            0x20

/* ack or nack */
#define LITHIUM_RESPONSE_NACK               0xFFFF
#define LITHIUM_RESPONSE_ACK                0x0A0A

#define LITHIUM_IS_ACK(uint16)              (uint16 == LITHIUM_RESPONSE_ACK)
#define LITHIUM_IS_NACK(uint16)             (uint16 == LITHIUM_RESPONSE_NACK)
#define LITHIUM_IS_ACK_OR_NACK(uint16)      (LITHIUM_IS_ACK(uint16) || LITHIUM_IS_NACK(uint16))

/* Command and Data Interface baud rate */
#define LITHIUM_CDI_BAUD_RATE_9600             0
#define LITHIUM_CDI_BAUD_RATE_19200            1
#define LITHIUM_CDI_BAUD_RATE_38400            2
#define LITHIUM_CDI_BAUD_RATE_76800            3
#define LITHIUM_CDI_BAUD_RATE_115200           4

/* RF baud rate */
#define LITHIUM_RF_BAUD_RATE_1200              0
#define LITHIUM_RF_BAUD_RATE_9600              1
#define LITHIUM_RF_BAUD_RATE_19200             2
#define LITHIUM_RF_BAUD_RATE_38400             3

/* LITHIUM RF MODULATION */
#define LITHIUM_RF_MODULATION_GFSK             0
#define LITHIUM_RF_MODULATION_AFSK             1
#define LITHIUM_RF_MODULATION_BPSK             2


/* FUNCTION_CONFIG bit values (WE NEED DOCS ON THIS!) */
#define LITHIUM_FC_PIN12_OFF_LOGIC_LOW        0
#define LITHIUM_FC_PIN12_25_SECOND_TOGGLE     1
#define LITHIUM_FC_PIN12_TX_PKT_TOGGLE        2
#define LITHIUM_FC_PIN12_RX_PKT_TOGGLE        3

#define LITHIUM_FC_PIN13_OFF_LOGIC_LOW        0
#define LITHIUM_FC_PIN13_RX_TX_SWITCH         4
#define LITHIUM_FC_PIN13_25_HZ_WDT            8
#define LITHIUM_FC_PIN13_RX_PKT_TOGGLE       12

#define LITHIUM_FC_PIN14_OFF_LOGIC_LOW        0
#define LITHIUM_FC_PIN14_DIO_OTA_KEY_ENABLE  16      
#define LITHIUM_FC_PIN14_DIO_OTA_PAT_A       32      
#define LITHIUM_FC_PIN14_DIO_OTA_PAT_B       48

#define LITHIUM_FC_RX_CRC_ENABLE             64  /* enable: 1 disable: 0 */
// #define LITHIUM_FC_TX_CRC_ENABLE            128  /* Not Implemented */

#define LITHIUM_FC_TMT_PKT_LOG              256  /* enable: 1 disable: 0 */
#define LITHIUM_FC_TMT_LOG_RATE             512  /* Logging Rate 0 1/10 Hz, 1 1 Hz, 2 2 Hz,3 4 Hz */
#define LITHIUM_FC_TMT_DUMP                2048  /* enable: 1 disable: 0 */

#define LITHIUM_FC_BC_PING_RETURN          4096
#define LITHIUM_FC_CODE_UPLOAD             8192
#define LITHIUM_FC_RADIO_RESET            16384

#define LITHIUM_FC_RESTORE_COMPLETE       32768

#define LITHIUM_FC2_RX_AFC					(1 << 0)
#define LITHIUM_FC2_TX_CW					(1 << 1)	/* don't use */
#define LITHIUM_FC2_RX_CW					(1 << 2)	/* don't use */
/* frame sizes and constants */

#define LITHIUM_SYNC_CHAR_1   'H'
#define LITHIUM_SYNC_CHAR_2   'e'

#define LITHIUM_BEACON_SECONDS_TO_VALUE(_seconds)	((_seconds)*4/10)	/* 2.5 seconds per digit. No floats */
#define LITHIUM_DEFAULT_POWER_AMP_LEVEL (200)		/* 1094mW @ 1200 / 1143 @ 9600 mW*/
#define LITHIUM_DEFAULT_FRONT_END_LEVEL	(63)

#define LITHIUM_AFSK_FREQUENCY_SHIFT_KHz	10
#define LITHIUM_FSK_FREQUENCY_SHIFT_KHz		5

// TODO move me into nvram_cubebug2.c
#define LITHIUM_CONFIGURATION_DEFAULT_VALUES { \
    .interface_baud_rate = LITHIUM_CDI_BAUD_RATE_9600,  \
        .tx_power_amp_level = LITHIUM_DEFAULT_POWER_AMP_LEVEL,                     \
        .rx_rf_baud_rate = LITHIUM_RF_BAUD_RATE_9600,   \
        .tx_rf_baud_rate = LITHIUM_RF_BAUD_RATE_9600,   \
        .rx_modulation = LITHIUM_RF_MODULATION_GFSK,    \
        .tx_modulation = LITHIUM_RF_MODULATION_GFSK,    \
        .rx_freq = 437445 + LITHIUM_FSK_FREQUENCY_SHIFT_KHz,                      \
        .tx_freq = 437445 + LITHIUM_FSK_FREQUENCY_SHIFT_KHz,                      \
        .source = "CQ    ",                             \
        .destination = "CUBEB2",                        \
        .tx_preamble = 0x03,                            \
        .tx_postamble = 0x00,                           \
        .function_config =								\
			/*LITHIUM_FC_RX_CRC_ENABLE  |  */ 				\
			LITHIUM_FC_BC_PING_RETURN |					\
			LITHIUM_FC_CODE_UPLOAD |					\
			LITHIUM_FC_RADIO_RESET,						\
		.function_config2 = LITHIUM_FC2_RX_AFC			\
}

#define LI_HEADER_BYTES(__direction, __cmd, __length)	\
		LITHIUM_SYNC_CHAR_1,	\
		LITHIUM_SYNC_CHAR_2,	\
		__direction,	\
		__cmd,	\
		((__length) >> 8) & 0xff, (__length) & 0xff, \
		(uint8_t)((__direction) + __cmd + (((__length) >> 8) & 0xff) + ((__length) & 0xff)),	\
		(uint8_t)(4*(__direction) + 3*__cmd + 2*(((__length) >> 8) & 0xff) + ((__length) & 0xff))

#define DECLARE_LI_HEADER(__direction, __cmd, __length)	DECLARE_FRAME_BYTES(LI_HEADER_BYTES(__direction, __cmd, __length))

/* The following two macros may be an art piece, but they can't be used.
 * Not to initialize frames in the stack, as the frames will usually need
 * to live past the life of the frame where they are declared, for example,
 * becuase they are placed in a queue to be used by other task */

/*
#define DECLARE_LI_CMD_0(__cmd)		DECLARE_LI_HEADER(LITHIUM_DIRECTION_INPUT, __cmd, 0)

#define DECLARE_LI_CMD_1(__cmd, __arg)		DECLARE_FRAME_BYTES( \
    LI_HEADER_BYTES(LITHIUM_DIRECTION_INPUT, __cmd, 1), \
	(__arg), \
	(uint8_t)(6*(LITHIUM_DIRECTION_INPUT) + 5*(__cmd) + 3*(1) + (__arg)), \
	(uint8_t)(18*(LITHIUM_DIRECTION_INPUT) + 15*(__cmd) + 9*(1) + (__arg)))
*/
/* data types */

typedef struct lithium_configuration_t {
    uint8_t interface_baud_rate;   /**< Radio Interface Baud Rate (9600=0x00) */
    uint8_t tx_power_amp_level;    /**< Tx Power Amp level (min = 0x00 max = 0xFF) */
    uint8_t rx_rf_baud_rate;       /**< Radio RX RF Baud Rate  */
    uint8_t tx_rf_baud_rate;       /**< Radio TX RF Baud Rate */
    uint8_t rx_modulation;         /**< 0x00 = GFSK); */
    uint8_t tx_modulation;         /**< (0x00 = GFSK); */
    uint32_t rx_freq;              /**< Channel Rx Frequency (ex: 45000000) */
    uint32_t tx_freq;              /**< Channel Tx Frequency (ex: 45000000) */
    unsigned char source[AX25_CALLSIGN_LEN];       /**< AX25 Mode Source Call Sign (default NOCALL) */
    unsigned char destination[AX25_CALLSIGN_LEN];  /**< AX25 Mode Destination Call Sign (default CQ) */
    uint16_t tx_preamble;          /**< AX25 Mode Tx Preamble Byte Length (0x00 = 20 flags) */
    uint16_t tx_postamble;         /**< AX25 Mode Tx Postamble Byte Length (0x00 = 20 flags) */
    uint16_t function_config;      /**< Radio Configuration Discrete Behaviors */
    uint16_t function_config2;     /**< Must be zero */
}  __attribute__((packed)) lithium_configuration_t;

typedef struct lithium_telemetry_t {
    uint16_t op_counter;
    int16_t msp430_temp;
    uint8_t time_count[3];
    uint8_t rssi;
    uint32_t bytes_received;
    uint32_t bytes_transmitted;
} __attribute__((packed)) lithium_telemetry_t;

typedef struct lithium_rf_configuration_t {
    uint8_t front_end_level;      /**< 0 to 63 Value */
    uint8_t tx_power_amp_level;   /**< 0 to 255 value, non-linear */
    uint32_t tx_frequency_offset; /**< Up to 20 kHz */
    uint32_t rx_frequency_offset; /**< Up to 20 kHz */
    uint8_t undocumented_FSK_setting; /* fuck fuck fuck. Defaults to zero */
} lithium_rf_configuration_t;

retval_t lithium_initialize(const channel_t * channel);

/** 
 * Gets a telemetry reading from the radio hardware
 * 
 * @param[out] telemetry 
 * @param[out] timestamp 
 * 
 * @return RV_SUCCESS, RV_TIMEOUT, RV_BUSY, RV_ILLEGAL, RV_ERROR
 */
retval_t lithium_get_telemetry(lithium_telemetry_t *telemetry);

/** 
 * Configures the device
 *
 * @param[in] config 
 * 
 * @return RV_SUCCESS, RV_TIMEOUT, RV_BUSY, RV_ILLEGAL, RV_ERROR
 */
retval_t lithium_set_configuration(const lithium_configuration_t *config);

/** 
 * @param[in] rf_config 
 * 
 * @return RV_SUCCESS, RV_TIMEOUT, RV_BUSY, RV_ILLEGAL, RV_ERROR
 */
retval_t lithium_set_rf_configuration(const lithium_rf_configuration_t *rf_config);

/** 
 * @param[out] config 
 * 
 * @return RV_SUCCESS, RV_TIMEOUT, RV_BUSY, RV_ILLEGAL, RV_ERROR
 */
retval_t lithium_get_configuration(lithium_configuration_t *config);

/** 
 * Writes current configuration to flash memory
 * 
 * @param md5 MD5 hash (sizeof == 16) of current configuration
 * 
 * @return 
 */
retval_t lithium_write_configuration_to_flash(void *md5_digest);

/** 
 * Sets beacon interval
 * 
 * @param interval_s in seconds. will be rounded down to the closest multiple of 2.5
 * 
 * @return 
 */
retval_t lithium_set_beacon_interval(unsigned int interval_s);

/** 
 * Set beacon contents
 * 
 * @param data 
 * 
 * @return 
 */
retval_t lithium_set_beacon_data(frame_t *data);

/** 
 * Sets TX power
 * 
 * @param power 
 * 
 * @return 
 */
retval_t lithium_set_tx_power(unsigned int power);


/** 
 * Receive from Lithium RX Queue
 * 
 * @param pFrame 
 * 
 * @return 
 */
retval_t lithium_recv_data(frame_t **pFrame);

/** 
 * transmit the contents of `data_frame` over radio
 * 
 * @param[in] data_frame
 * 
 * @return 
 */
retval_t lithium_send_data(frame_t *data);

retval_t lithium_set_tx_bps(uint8_t tx_bps);
retval_t lithium_set_config_bps(lithium_configuration_t *config, uint8_t tx_bps);

retval_t lithium_noop(void);
retval_t lithium_reset(void);

void lithium_deinitialize(void);
void lithium_workaround_enable(void);
void lithium_workaround_disable(void);

const char *lithium_cmd_s(lithium_cmd_t cmd);

#endif
