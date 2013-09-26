#include <canopus/drivers/power/ina209_powerdomain.h>
#include <canopus/drivers/power/ina209.h>

#define RSHUNT				0.1f

retval_t PD_ina_initialize(const channel_t *const channel, float voltage, float criticalPowerLevel_mW) {
	ina209_outputPinCfg_t cfg;
	uint16_t status;
	float Current_LSB = 100E-5f;
	uint16_t cal = 0.04096f/(Current_LSB*RSHUNT);	//FIXME: Very important to keep the current_lsb to 10E-5 because the driver is harcoded. Very ugly

	PD_ina_turnOff(channel);

	ina209_setShuntVoltageResolution(channel, INA209_ADC_12bit);
	ina209_setBusVoltageResolution(channel, INA209_ADC_12bit);
	ina209_setBusVoltageRange(channel, INA209_RANGE_16V);
	ina209_setPGAValue(channel, INA209_PGA_40mV);		// INA209_PGA_160mV for 12v

	ina209_writeRegister(channel, INA209_CALIBRATION, cal);
	ina209_setOperatingMode(channel, INA209_MODE_SHUNT_BUS_CONTINUOUS);

	// Limits
	ina209_setOverlimitBusOverVoltage(channel, voltage * 1.1f);		// +10%
	ina209_setWarningBusOverVoltage(channel, voltage * 1.05f);		// +5%
	ina209_setWarningBusUnderVoltage(channel, voltage * 0.95f);		// -5%
	ina209_setOverlimitBusUnderVoltage(channel, voltage * 0.9f);	// -10%

	ina209_setOverlimitPower(channel, criticalPowerLevel_mW * 0.9f);			// critical_mW = 1500 / 25000
	ina209_setWarningPower(channel, criticalPowerLevel_mW * 0.8f);

	ina209_setCriticalDACpos(channel, criticalPowerLevel_mW        / 1000.0f / voltage * RSHUNT);
	ina209_setWarningShuntPos(channel, criticalPowerLevel_mW * 0.8 / 1000.0f / voltage * RSHUNT);
	ina209_setWarningShuntNeg(channel, 0.000);
	ina209_setCriticalDACneg(channel, 255.0E-3); //unidirectional application, Datasheet pg 17 left column at the bottoms

	// The latched is fundamental and the polarity is because of the circuit used on the switch.

	cfg.mode = PIN_ENABLED;
	cfg.polarity = ACTIVE_HIGH;
	cfg.latch = LATCHED;
	cfg.SMBusMask = 0;

	cfg.pin = WARNING_PIN;
	ina209_configureOutput(channel, &cfg);

	cfg.pin = OVERLIMIT_PIN;
	ina209_configureOutput(channel, &cfg);

	PD_ina_turnOff(channel);

	// Configure filter to support some peak consumptions on in-rush (specially on SVIP)
	// Only required for 12v, but harmless on 3v3 for our current usage
	ina209_writeOnRegister(channel, INA209_CRITICAL_DAC_NEG, INA209_CRITICAL_DAC_CF_MASK, (15<<INA209_CRITICAL_DAC_CF_SHIFT));	// 0.96ms only for 12v
	return ina209_getStatus(channel, &status);
}

retval_t PD_ina_turnOn(const channel_t *const channel) {
	// ToDo: Must clear critical status bit (it's latched)
	ina209_outputPinCfg_t cfg;

	cfg.pin = CRITICAL_PIN;
	cfg.mode = PIN_ENABLED;
	cfg.polarity = ACTIVE_HIGH;
	cfg.latch = LATCHED;
	cfg.SMBusMask = 0;
	return ina209_configureOutput(channel, &cfg);
}

retval_t PD_ina_turnOff(const channel_t *const channel) {
	ina209_outputPinCfg_t cfg;

	cfg.pin = CRITICAL_PIN;
	cfg.mode = PIN_DISABLED;
	cfg.polarity = ACTIVE_LOW;
	cfg.latch = LATCHED;
	cfg.SMBusMask = 0;
	return ina209_configureOutput(channel, &cfg);
}
