#ifndef _ADIS16xxx_H_
#define _ADIS16xxx_H_

#include <canopus/drivers/channel.h>

typedef struct adis1640x_struct {
	channel_t *ch_spi;
} adis1640x;

typedef struct adis1640x_data_burst_raw_struct {
	uint16_t power_supply_voltage;
	uint16_t gyro_x;
	uint16_t gyro_y;
	uint16_t gyro_z;
	uint16_t accel_x;
	uint16_t accel_y;
	uint16_t accel_z;
	uint16_t mag_x;
	uint16_t mag_y;
	uint16_t mag_z;
	uint16_t temp;
	uint16_t adc;
} adis1640x_data_burst_raw;

#define IMU16400_DEVICE_ID_ENG 0x4010
#define IMU16400_DEVICE_ID_FLIGHT 0x4015

#define ADIS16400_GYRO_SCALE_300 ((double).05)
#define ADIS16400_GYRO_SCALE_150 ((double).025)
#define ADIS16400_GYRO_SCALE_075 ((double).0125)

#define ADIS16400_MAGN_SCALE ((double) 0.5)

/* See page 12 first column of Rev.B documentation for ADIS16400/ADIS16405 IMU.*/
#define MASK_ND_EA (ADIS16400_ERROR_ACTIVE | ADIS16400_NEW_DATA)
#define RAW2LSB14(__raw) (((~MASK_ND_EA & __raw)>=8192)?((~MASK_ND_EA & __raw)-16384):(~MASK_ND_EA & __raw))

#define MASK_ND_EA_12 (ADIS16400_ERROR_ACTIVE | ADIS16400_NEW_DATA | 0x3000)
#define RAW2LSB12(__raw) (((~MASK_ND_EA_12 & __raw)>=2048)?((~MASK_ND_EA_12 & __raw)-4096):(~MASK_ND_EA_12 & __raw))

#define BIT(nr) (1UL << (nr))
#define LO8(u16) ((u16) & 0xff)
#define HI8(u16) (((u16) >> 8) & 0xff)

#define ADIS16400_WRITE_REG(r) (0x80 | (r))

#define ADIS16400_STARTUP_DELAY	290 /* ms */
#define ADIS16400_SELF_TEST_DELAY 100 /* ms */

#define ADIS16400_FLASH_CNT  0x00 /* Flash memory write count */
#define ADIS16400_SUPPLY_OUT 0x02 /* Power supply measurement */
#define ADIS16400_XGYRO_OUT 0x04 /* X-axis gyroscope output */
#define ADIS16400_YGYRO_OUT 0x06 /* Y-axis gyroscope output */
#define ADIS16400_ZGYRO_OUT 0x08 /* Z-axis gyroscope output */
#define ADIS16400_XACCL_OUT 0x0A /* X-axis accelerometer output */
#define ADIS16400_YACCL_OUT 0x0C /* Y-axis accelerometer output */
#define ADIS16400_ZACCL_OUT 0x0E /* Z-axis accelerometer output */
#define ADIS16400_XMAGN_OUT 0x10 /* X-axis magnetometer measurement */
#define ADIS16400_YMAGN_OUT 0x12 /* Y-axis magnetometer measurement */
#define ADIS16400_ZMAGN_OUT 0x14 /* Z-axis magnetometer measurement */
#define ADIS16400_TEMP_OUT  0x16 /* Temperature output */
#define ADIS16400_AUX_ADC   0x18 /* Auxiliary ADC measurement */

#define ADIS16350_XTEMP_OUT 0x10 /* X-axis gyroscope temperature measurement */
#define ADIS16350_YTEMP_OUT 0x12 /* Y-axis gyroscope temperature measurement */
#define ADIS16350_ZTEMP_OUT 0x14 /* Z-axis gyroscope temperature measurement */

/* Calibration parameters */
#define ADIS16400_XGYRO_OFF 0x1A /* X-axis gyroscope bias offset factor */
#define ADIS16400_YGYRO_OFF 0x1C /* Y-axis gyroscope bias offset factor */
#define ADIS16400_ZGYRO_OFF 0x1E /* Z-axis gyroscope bias offset factor */
#define ADIS16400_XACCL_OFF 0x20 /* X-axis acceleration bias offset factor */
#define ADIS16400_YACCL_OFF 0x22 /* Y-axis acceleration bias offset factor */
#define ADIS16400_ZACCL_OFF 0x24 /* Z-axis acceleration bias offset factor */
#define ADIS16400_XMAGN_HIF_OFF 0x26 /* X-axis magnetometer, hard-iron factor */
#define ADIS16400_YMAGN_HIF_OFF 0x28 /* Y-axis magnetometer, hard-iron factor */
#define ADIS16400_ZMAGN_HIF_OFF 0x2A /* Z-axis magnetometer, hard-iron factor */
#define ADIS16400_XMAGN_SIF_OFF 0x2C /* X-axis magnetometer, soft-iron factor */
#define ADIS16400_YMAGN_SIF_OFF 0x2E /* Y-axis magnetometer, soft-iron factor */
#define ADIS16400_ZMAGN_SIF_OFF 0x30 /* Z-axis magnetometer, soft-iron factor */

#define ADIS16400_GPIO_CTRL 0x32 /* Auxiliary digital input/output control */
#define ADIS16400_MSC_CTRL  0x34 /* Miscellaneous control */
#define ADIS16400_SMPL_PRD  0x36 /* Internal sample period (rate) control */
#define ADIS16400_SENS_AVG  0x38 /* Dynamic range and digital filter control */
#define ADIS16400_SLP_CNT   0x3A /* Sleep mode control */
#define ADIS16400_DIAG_STAT 0x3C /* System status */

/* Alarm functions */
#define ADIS16400_GLOB_CMD  0x3E /* System command */
#define ADIS16400_ALM_MAG1  0x40 /* Alarm 1 amplitude threshold */
#define ADIS16400_ALM_MAG2  0x42 /* Alarm 2 amplitude threshold */
#define ADIS16400_ALM_SMPL1 0x44 /* Alarm 1 sample size */
#define ADIS16400_ALM_SMPL2 0x46 /* Alarm 2 sample size */
#define ADIS16400_ALM_CTRL  0x48 /* Alarm control */
#define ADIS16400_AUX_DAC   0x4A /* Auxiliary DAC data */

#define ADIS16400_PRODUCT_ID 0x56 /* Product identifier */

#define ADIS16400_ERROR_ACTIVE		(1<<15)
#define ADIS16400_NEW_DATA			(1<<14)

/* MSC_CTRL */
#define ADIS16400_MSC_CTRL_MEM_TEST			(1<<11)
#define ADIS16400_MSC_CTRL_INT_SELF_TEST	(1<<10)
#define ADIS16400_MSC_CTRL_NEG_SELF_TEST	(1<<9)
#define ADIS16400_MSC_CTRL_POS_SELF_TEST	(1<<8)
#define ADIS16400_MSC_CTRL_GYRO_BIAS		(1<<7)
#define ADIS16400_MSC_CTRL_ACCL_ALIGN		(1<<6)
#define ADIS16400_MSC_CTRL_DATA_RDY_EN		(1<<2)
#define ADIS16400_MSC_CTRL_DATA_RDY_POL_HIGH	(1<<1)
#define ADIS16400_MSC_CTRL_DATA_RDY_DIO2	(1<<0)

#define ADIS16400_MSC_CTRL_DATA_RDY_BITS (ADIS16400_MSC_CTRL_DATA_RDY_EN | ADIS16400_MSC_CTRL_DATA_RDY_POL_HIGH | ADIS16400_MSC_CTRL_DATA_RDY_DIO2)

/* SMPL_PRD */
#define ADIS16400_SMPL_PRD_TIME_BASE	(1<<7)
#define ADIS16400_SMPL_PRD_DIV_MASK		0x7F

/* DIAG_STAT */
#define ADIS16400_DIAG_STAT_ZACCL_FAIL	(1<<15)
#define ADIS16400_DIAG_STAT_YACCL_FAIL	(1<<14)
#define ADIS16400_DIAG_STAT_XACCL_FAIL	(1<<13)
#define ADIS16400_DIAG_STAT_XGYRO_FAIL	(1<<12)
#define ADIS16400_DIAG_STAT_YGYRO_FAIL	(1<<11)
#define ADIS16400_DIAG_STAT_ZGYRO_FAIL	(1<<10)
#define ADIS16400_DIAG_STAT_ALARM2		(1<<9)
#define ADIS16400_DIAG_STAT_ALARM1		(1<<8)
#define ADIS16400_DIAG_STAT_FLASH_CHK	(1<<6)
#define ADIS16400_DIAG_STAT_SELF_TEST	(1<<5)
#define ADIS16400_DIAG_STAT_OVERFLOW	(1<<4)
#define ADIS16400_DIAG_STAT_SPI_FAIL	(1<<3)
#define ADIS16400_DIAG_STAT_FLASH_UPT	(1<<2)
#define ADIS16400_DIAG_STAT_POWER_HIGH	(1<<1)
#define ADIS16400_DIAG_STAT_POWER_LOW	(1<<0)

/* GLOB_CMD */
#define ADIS16400_GLOB_CMD_SW_RESET		(1<<7)
#define ADIS16400_GLOB_CMD_P_AUTO_NULL	(1<<4)
#define ADIS16400_GLOB_CMD_FLASH_UPD	(1<<3)
#define ADIS16400_GLOB_CMD_DAC_LATCH	(1<<2)
#define ADIS16400_GLOB_CMD_FAC_CALIB	(1<<1)
#define ADIS16400_GLOB_CMD_AUTO_NULL	(1<<0)

/* SLP_CNT */
#define ADIS16400_SLP_CNT_POWER_OFF	(1<<8)

/* BURST MODE */
#define ADIS1640x_BURST_CMD 0x3e
#define ADIS1640x_BURST_0s 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
/*                         1   2   3   4   5   6   7   8   9   0   1   2  */

/* SENS_AVG */
#define ADIS16400_SENS_AVG_RANGE 0x0700
#define ADIS16400_SENS_AVG_TAPS  0x0007

#define ADIS16400_GYRO_OFF_MASK 0x1FFF
#define ADIS16400_ACCL_OFF_MASK 0xFFF
#define ADIS16400_MAGN_HIF_OFF_MASK 0x3FFF
#define ADIS16400_MAGN_SIF_OFF_MASK 0xFFF

/*
 * A register for this function is 16 bits, its address is the address
 * of the lower byte. This function will do 4 SPI one byte transactions.
 *
 * See page 11 of Rev.B documentation for ADIS16400/ADIS16405 IMU.
 *
 * @param imu
 * @param reg_nr Address of the lower byte of the register to read.
 * @param [out] value The 16 bits of the value of that register.
 *
 */
retval_t adis1640x_read_register(const channel_t *const channel, uint8_t reg_nr, uint16_t *const value);

/*
 * A register for this function is 16 bits, its address is the address
 * of the lower byte. This function will do 4 SPI one byte transactions,
 * a register write on the IMU uses 2 byte transactions for each the
 * upper and lower 8 bits. The first byte is 0x80 | reg_nr and the other
 * byte is the value.
 *
 * See page 10 of Rev.B documentation for ADIS16400/ADIS16405 IMU.
 *
 * @param imu
 * @param reg_nr Address of the lower byte of the register to write.
 * @param value 16 bit value to write on the register.
 *
 */
retval_t adis1640x_write_register(const channel_t *const channel, uint8_t reg_nr, uint16_t value);

/*
 * Burst mode read. Reads all sensors, plus ADC and voltage.
 *
 * See page 10 and 11 of Rev.B documentation for ADIS16400/ADIS16405 IMU.
 *
 * @param imu
 * @param [out] value struct to hold all values.
 *
 */
retval_t adis1640x_read_burst(const channel_t *const channel, adis1640x_data_burst_raw * const value);

/*
 * Transform each possible bit flag of diag_stat into an error message.
 * See page 15 of Rev.B documentation for ADIS16400/ADIS16405 IMU.
 *
 * @param bit number of enabled bit.
 *
 * @retval String with error message.
 */
char const* adis1640x_diag_stat_bit_to_string(uint8_t bit);

#endif /* _ADIS16xxx_H_ */

