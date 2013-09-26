#ifndef INA209_H_
#define INA209_H_

#include <canopus/types.h>
#include <canopus/drivers/channel.h>

/* register definitions */
#define INA209_CONFIGURATION			0x00
#define INA209_STATUS					0x01
#define INA209_STATUS_MASK				0x02
#define INA209_SHUNT_VOLTAGE			0x03
#define INA209_BUS_VOLTAGE				0x04
#define INA209_POWER					0x05
#define INA209_CURRENT					0x06
#define INA209_SHUNT_VOLTAGE_POS_PEAK	0x07
#define INA209_SHUNT_VOLTAGE_NEG_PEAK	0x08
#define INA209_BUS_VOLTAGE_MAX_PEAK		0x09
#define INA209_BUS_VOLTAGE_MIN_PEAK		0x0A
#define INA209_POWER_PEAK				0x0B
#define INA209_SHUNT_VOLTAGE_POS_WARN	0x0c
#define INA209_SHUNT_VOLTAGE_NEG_WARN	0x0D
#define INA209_POWER_WARN				0x0E
#define INA209_BUS_VOLTAGE_OVER_WARN	0x0F
#define INA209_BUS_VOLTAGE_UNDER_WARN	0x10
#define INA209_POWER_OVER_LIMIT			0x11
#define INA209_BUS_VOLTAGE_OVER_LIMIT	0x12
#define INA209_BUS_VOLTAGE_UNDER_LIMIT	0x13
#define INA209_CRITICAL_DAC_POS			0x14
#define INA209_CRITICAL_DAC_NEG			0x15
#define INA209_CALIBRATION				0x16

// Bit mask definitions

//------------------------------------------------------
// STATUS register
#define INA209_WARNING_BUS_OVER_VOLT			(1<<15)
#define INA209_WARNING_BUS_UNDER_VOLT			(1<<14)
#define INA209_WARNING_POWER					(1<<13)
#define INA209_WARNING_SHUNT_POS_VOLT			(1<<12)
#define INA209_WARNING_SHUNT_NEG_VOLT			(1<<11)
#define INA209_OVER_LIMIT_BUS_OVER_VOLT			(1<<10)
#define INA209_OVER_LIMIT_BUS_UNDER_VOLT		(1<<9)
#define INA209_OVER_LIMIT_POWER					(1<<8)
#define INA209_CRITICAL_SHUNT_POS_VOLT			(1<<7)
#define INA209_CRITICAL_SHUNT_NEG_VOLT			(1<<6)
#define INA209_CONVERSION_READY					(1<<5)
#define INA209_SMBus_ALERT						(1<<4)
#define INA209_MATH_OVERFLOW					(1<<3)

// Alert/Mask control register
#define INA209_WARNING_PIN						(1<<0)
#define INA209_OVER_LIMIT_PIN					(1<<1)
#define INA209_CRITICAL_DAC_PIN					(1<<2)
//------------------------------------------------------
// types

typedef enum ina209_VoltageRange_t {
	INA209_RANGE_16V = 0 << 13,
	INA209_RANGE_32V = 1 << 13
} ina209_VoltageRange_t;
#define INA209_VOLTAGE_RANGE_MASK	(0x01<<13)

typedef enum ina209_PGA_t {
	INA209_PGA_40mV =  0 << 11,
	INA209_PGA_80mV =  1 << 11,
	INA209_PGA_160mV = 2 << 11,
	INA209_PGA_320mV = 3 << 11
} ina209_PGA_t;
#define INA209_PGA_MASK	(0x03<<11)

typedef enum ina209_ADCResolution_t {
	INA209_ADC_9bit = 0,
	INA209_ADC_10bit = 1,
	INA209_ADC_11bit = 2,
	INA209_ADC_12bit = 3,
	INA209_ADC_12bit_aver1 = 8,
	INA209_ADC_12bit_aver2 = 9,
	INA209_ADC_12bit_aver4 = 10,
	INA209_ADC_12bit_aver8 = 11,
	INA209_ADC_12bit_aver16 = 12,
	INA209_ADC_12bit_aver32 = 13,
	INA209_ADC_12bit_aver64 = 14,
	INA209_ADC_12bit_aver128 = 15,
} ina209_ADCResolution_t;

#define INA209_SADC_MASK	(0x0F<<3)
#define INA209_BADC_MASK	(0x0F<<7)

#define INA209_CRITICAL_DAC_CF_SHIFT 4
#define INA209_CRITICAL_DAC_CF_MASK	(0x0F << INA209_CRITICAL_DAC_CF_SHIFT)

typedef enum ina209_operatingMode_t {
	INA209_MODE_POWERDOWN = 0,
	INA209_MODE_SHUNT_TRIGGERED,
	INA209_MODE_BUS_TRIGGERED,
	INA209_MODE_SHUNT_BUS_TRIGGERED,
	INA209_MODE_ADC_OFF,
	INA209_MODE_SHUNT_CONTINUOUS,
	INA209_MODE_BUS_CONTINUOUS,
	INA209_MODE_SHUNT_BUS_CONTINUOUS,
} ina209_operatingMode_t;
#define INA209_MODE_MASK	(0x03)

typedef enum ina209_outputPin_t {
	WARNING_PIN=0,
	OVERLIMIT_PIN,
	CRITICAL_PIN,
	ALERT_PIN
} ina209_outputPin_t;

typedef enum ina209_outputMode_t {
	PIN_DISABLED = 0,
	PIN_ENABLED
} ina209_outputMode_t;

typedef enum ina209_outputPolarity_t {
	ACTIVE_LOW = 0,
	ACTIVE_HIGH
} ina209_outputPolarity_t;

typedef enum ina209_outputLatch_t {
	TRANSPARENT = 0,
	LATCHED
} ina209_outputLatch_t;

typedef struct ina209_outputPinCfg_t {
	ina209_outputPin_t		pin;
	ina209_outputMode_t		mode;
	ina209_outputPolarity_t	polarity;
	ina209_outputLatch_t	latch;
	uint16_t				SMBusMask;
}ina209_outputPinCfg_t;

// Defines
#define INA209_CONFIGURATION_DEFAULT	0x399F

// Low level function
retval_t ina209_init(const channel_t *const channel);
retval_t ina209_readRegister(const channel_t *const channel, uint8_t regAdress, uint16_t * buffer);
retval_t ina209_writeRegister(const channel_t *const channel, uint8_t regAdress, uint16_t buffer);
retval_t ina209_setBitsOnRegister(const channel_t *const channel, uint8_t regAddress, uint16_t bitmask);
retval_t ina209_clearBitsOnRegister(const channel_t *const channel, uint8_t regAddress, uint16_t bitmask);
retval_t ina209_writeOnRegister(const channel_t *const channel, uint8_t regAddress, uint16_t bitmask, uint16_t value);

// Data registers
retval_t ina209_getBusVoltage(const channel_t *const channel, float * busVoltage_v);
retval_t ina209_getShuntVoltage(const channel_t *const channel, float * shuntVoltage_v );
retval_t ina209_getCurrent(const channel_t *const channel, float * current_ma);
retval_t ina209_getPower(const channel_t *const channel, float * power_mw);

// Configuration register
retval_t ina209_setOperatingMode(const channel_t *const channel,ina209_operatingMode_t mode);
retval_t ina209_setShuntVoltageResolution(const channel_t *const channel,ina209_ADCResolution_t resolution);
retval_t ina209_setBusVoltageResolution(const channel_t *const channel,ina209_ADCResolution_t resolution);
retval_t ina209_setPGAValue(const channel_t *const channel,ina209_PGA_t range);
retval_t ina209_setBusVoltageRange(const channel_t *const channel,ina209_VoltageRange_t range);
retval_t ina209_reset(const channel_t *const channel);

// Peak-hold registers
retval_t ina209_getPeakShuntP(const channel_t *const channel, float * peak_v);
retval_t ina209_getPeakShuntN(const channel_t *const channel, float * peak_v);
retval_t ina209_getPeakBusMax(const channel_t *const channel, float * peak_v);
retval_t ina209_getPeakBusMin(const channel_t *const channel, float * peak_v);
retval_t ina209_getPeakPower(const channel_t *const channel, float * peak_mW);
// TODO: It would be nice to include the function to reset this register via de bit 0 of each register

// Status register
retval_t ina209_getStatus(const channel_t *const channel, uint16_t * status);

// Warning, over-limit and critical watchdog levels register
retval_t ina209_setWarningShuntPos(const channel_t *const channel, float voltage_v);
retval_t ina209_setWarningShuntNeg(const channel_t *const channel, float voltage_v);
retval_t ina209_setWarningPower(const channel_t *const channel, float power_mW);
retval_t ina209_setWarningBusOverVoltage(const channel_t *const channel, float voltage_v);
retval_t ina209_setWarningBusUnderVoltage(const channel_t *const channel, float voltage_v);

retval_t ina209_setOverlimitPower(const channel_t *const channel, float power_mW);
retval_t ina209_setOverlimitBusOverVoltage(const channel_t *const channel, float voltage_v);
retval_t ina209_setOverlimitBusUnderVoltage(const channel_t *const channel, float voltage_v);
retval_t ina209_setCriticalDACpos(const channel_t *const channel, float voltage_v);
retval_t ina209_setCriticalDACneg(const channel_t *const channel, float voltage_v);

//retval_t ina209_activatePins(const channel_t *const channel, uint16_t mask);
//retval_t ina209_deactivatePins(const channel_t *const channel, uint16_t mask);

retval_t ina209_configureOutput(const channel_t *const channel, ina209_outputPinCfg_t * cfg);
//TODO: Implement SMBUs Alert part of the mask alert register
//TODO: Implement de setter/getter for delay time

//------------------------- Private -------------------------------
float ina209_from_reg(const uint8_t reg, const uint16_t val); //XXX: declare as private?
uint16_t ina209_to_reg(uint8_t reg, float val); // XXX: declare as private?

#endif /* INA209_H_ */
