#include <canopus/types.h>
#include <canopus/drivers/power/ina209.h>
#include <canopus/drivers/channel.h>
#include <canopus/frame.h>
#include <canopus/logging.h>

retval_t ina209_init(const channel_t *const channel) {
	uint16_t value;
	retval_t rv;

	ina209_reset(channel);
	rv = ina209_readRegister(channel, INA209_CONFIGURATION, &value);
	if (RV_SUCCESS == rv) {
		if (INA209_CONFIGURATION_DEFAULT != value) {
			rv = RV_ERROR;
		}
	}

	return rv;
}

// --------------------------------------------------------------------------------------------
// Data registers
// --------------------------------------------------------------------------------------------
retval_t ina209_getBusVoltage(const channel_t *const channel, float * busVoltage_v) {
	uint16_t buffer;
	retval_t ret=RV_ILLEGAL;

	ret = ina209_readRegister(channel,INA209_BUS_VOLTAGE,&buffer);

	if (ret==RV_SUCCESS){
		*busVoltage_v = ina209_from_reg(INA209_BUS_VOLTAGE,buffer);
	}
	return ret;
}

retval_t ina209_getShuntVoltage(const channel_t *const channel, float * shuntVoltage_v ){
	int16_t buffer;
	retval_t ret=RV_ILLEGAL;

	ret = ina209_readRegister(channel,INA209_SHUNT_VOLTAGE,(uint16_t*)&buffer);
	if (ret==RV_SUCCESS){
		*shuntVoltage_v = ina209_from_reg(INA209_SHUNT_VOLTAGE,buffer);
	}
	return ret;
}

retval_t ina209_getCurrent(const channel_t *const channel, float * current_ma){
	uint16_t buffer;
	retval_t ret=RV_ILLEGAL;

	ret = ina209_readRegister(channel,INA209_CURRENT,&buffer);
	if (ret==RV_SUCCESS){
		*current_ma = ina209_from_reg(INA209_CURRENT,buffer);
	}

	return ret;
}

retval_t ina209_getPower(const channel_t *const channel, float * power_mw){
	uint16_t buffer;
	retval_t ret=RV_ILLEGAL;

	ret = ina209_readRegister(channel,INA209_POWER,&buffer);
	if (ret==RV_SUCCESS){
		*power_mw = ina209_from_reg(INA209_POWER,buffer);
	}
	return ret;
}

// --------------------------------------------------------------------------------------------
// Configuration register
// --------------------------------------------------------------------------------------------

retval_t ina209_reset(const channel_t *const channel){
	return ina209_writeRegister(channel, INA209_CONFIGURATION, (1<<15));
}

retval_t ina209_setBusVoltageRange(const channel_t *const channel,ina209_VoltageRange_t range){
	retval_t res;

	res = ina209_writeOnRegister(channel,INA209_CONFIGURATION,INA209_VOLTAGE_RANGE_MASK,range);
	return res;
}

retval_t ina209_setPGAValue(const channel_t *const channel,ina209_PGA_t range){
	return ina209_writeOnRegister(channel,INA209_CONFIGURATION, INA209_PGA_MASK, range);
}

retval_t ina209_setBusVoltageResolution(const channel_t *const channel,ina209_ADCResolution_t resolution){
	return ina209_writeOnRegister(channel,INA209_CONFIGURATION, INA209_BADC_MASK, resolution<<7);
}

retval_t ina209_setShuntVoltageResolution(const channel_t *const channel,ina209_ADCResolution_t resolution){
	return ina209_writeOnRegister(channel,INA209_CONFIGURATION, INA209_SADC_MASK, resolution<<3);
}

retval_t ina209_setOperatingMode(const channel_t *const channel,ina209_operatingMode_t mode){
	return ina209_writeOnRegister(channel, INA209_CONFIGURATION, INA209_MODE_MASK, mode);
}


// --------------------------------------------------------------------------------------------
// Status register
// --------------------------------------------------------------------------------------------
retval_t ina209_getStatus(const channel_t *const channel, uint16_t * status){
	return ina209_readRegister(channel,INA209_STATUS,status);
}

// --------------------------------------------------------------------------------------------
// Peak hold registers
// --------------------------------------------------------------------------------------------

retval_t ina209_getPeakShuntP(const channel_t *const channel, float * peak_v){
	retval_t ret;
	int16_t buffer;
	ret = ina209_readRegister(channel,INA209_SHUNT_VOLTAGE_POS_PEAK,(uint16_t*)&buffer);
	if (ret==RV_SUCCESS){
		*peak_v = ina209_from_reg(INA209_SHUNT_VOLTAGE_POS_PEAK,buffer);
	}
	return ret;
}

retval_t ina209_getPeakShuntN(const channel_t *const channel, float * peak_v){
	retval_t ret;
	int16_t buffer;
	ret = ina209_readRegister(channel,INA209_SHUNT_VOLTAGE_NEG_PEAK,(uint16_t*)&buffer);
	if (ret==RV_SUCCESS){
		*peak_v = ina209_from_reg(INA209_SHUNT_VOLTAGE_NEG_PEAK,buffer);
	}
	return ret;
}

retval_t ina209_getPeakBusMax(const channel_t *const channel, float * peak_v){
	retval_t ret;
	uint16_t buffer;
	ret = ina209_readRegister(channel,INA209_BUS_VOLTAGE_MAX_PEAK,&buffer);
	if (ret==RV_SUCCESS){
		*peak_v = ina209_from_reg(INA209_BUS_VOLTAGE_MAX_PEAK,buffer);
	}
	return ret;
}

retval_t ina209_getPeakBusMin(const channel_t *const channel, float * peak_v){
	retval_t ret;
	uint16_t buffer;
	ret = ina209_readRegister(channel,INA209_BUS_VOLTAGE_MIN_PEAK,&buffer);
	if (ret==RV_SUCCESS){
		*peak_v = ina209_from_reg(INA209_BUS_VOLTAGE_MIN_PEAK,buffer);
	}
	return ret;
}

retval_t ina209_getPeakPower(const channel_t *const channel, float * peak_mW){
	retval_t ret;
	uint16_t buffer;
	ret = ina209_readRegister(channel,INA209_POWER_PEAK,&buffer);
	if (ret==RV_SUCCESS){
		*peak_mW = ina209_from_reg(INA209_POWER_PEAK, buffer);
	}
	return ret;
}

// --------------------------------------------------------------------------------------------
// Warning, over limit and critical registers
// --------------------------------------------------------------------------------------------

// Warning, over-limit and critical watchdog levels register
retval_t ina209_setWarningShuntPos(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	buffer = ina209_to_reg(INA209_SHUNT_VOLTAGE_POS_WARN,voltage_v);
	return ina209_writeRegister(channel,INA209_SHUNT_VOLTAGE_POS_WARN,buffer);
}

retval_t ina209_setWarningShuntNeg(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	buffer = ina209_to_reg(INA209_SHUNT_VOLTAGE_NEG_WARN,voltage_v);
	return ina209_writeRegister(channel,INA209_SHUNT_VOLTAGE_NEG_WARN,buffer);
}

retval_t ina209_setWarningPower(const channel_t *const channel, float power_mW){
	uint16_t buffer;
	buffer = ina209_to_reg(INA209_POWER_WARN,power_mW);
	return ina209_writeRegister(channel,INA209_POWER_WARN,buffer);
}

retval_t ina209_setWarningBusOverVoltage(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	retval_t ret;
	// This function has to read the first 3 bits not to modify the original register
	// FIXME: could be made via writeOnRegister function
	ret = ina209_readRegister(channel,INA209_BUS_VOLTAGE_OVER_WARN,&buffer);

	if (ret==RV_SUCCESS){
		buffer |= ina209_to_reg(INA209_BUS_VOLTAGE_OVER_WARN,voltage_v);
		ret = ina209_writeRegister(channel,INA209_BUS_VOLTAGE_OVER_WARN,buffer);
	}

	return ret;
}

retval_t ina209_setWarningBusUnderVoltage(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	buffer = ina209_to_reg(INA209_BUS_VOLTAGE_UNDER_WARN,voltage_v);
	return ina209_writeRegister(channel,INA209_BUS_VOLTAGE_UNDER_WARN,buffer);
}

retval_t ina209_setOverlimitPower(const channel_t *const channel, float power_mW){
	uint16_t buffer;
	buffer = ina209_to_reg(INA209_POWER_OVER_LIMIT,power_mW);
	return ina209_writeRegister(channel,INA209_POWER_OVER_LIMIT,buffer);
}

retval_t ina209_setOverlimitBusOverVoltage(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	retval_t ret;
	// This function has to read the first 3 bits not to modify the original register
	// FIXME: could be made via writeOnRegister function
	ret = ina209_readRegister(channel,INA209_BUS_VOLTAGE_OVER_LIMIT,&buffer);

	if (ret==RV_SUCCESS){
		buffer |= ina209_to_reg(INA209_BUS_VOLTAGE_OVER_LIMIT,voltage_v);
		ret = ina209_writeRegister(channel,INA209_BUS_VOLTAGE_OVER_LIMIT,buffer);
	}

	return ret;
}
retval_t ina209_setOverlimitBusUnderVoltage(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	buffer = ina209_to_reg(INA209_BUS_VOLTAGE_UNDER_LIMIT,voltage_v);
	return ina209_writeRegister(channel,INA209_BUS_VOLTAGE_UNDER_LIMIT,buffer);
}

retval_t ina209_setCriticalDACpos(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	retval_t ret;
	// This function has to read the first 3 bits not to modify the original register
	// FIXME: could be made via writeOnRegister function
	ret = ina209_readRegister(channel,INA209_CRITICAL_DAC_POS,&buffer);

	if (ret==RV_SUCCESS){ //FIXME: ERROR NO BORRA LO ANTERIOR, SI O SI USAR writeOnRegister
		buffer |= ina209_to_reg(INA209_CRITICAL_DAC_POS,voltage_v);
		ret = ina209_writeRegister(channel,INA209_CRITICAL_DAC_POS,buffer);
	}

	return ret;
}

retval_t ina209_setCriticalDACneg(const channel_t *const channel, float voltage_v){
	uint16_t buffer;
	retval_t ret;
	// This function has to read the first 3 bits not to modify the original register
	// FIXME: could be made via writeOnRegister function
	ret = ina209_readRegister(channel,INA209_CRITICAL_DAC_NEG,&buffer);

	if (ret==RV_SUCCESS){
		buffer |= ina209_to_reg(INA209_CRITICAL_DAC_NEG,voltage_v);
		ret = ina209_writeRegister(channel,INA209_CRITICAL_DAC_NEG,buffer);
	}

	return ret;}

/*
retval_t ina209_activatePins(const channel_t *const channel, uint16_t mask){
	return ina209_setBitsOnRegister(deviceAddress,INA209_STATUS_MASK,mask);
}

retval_t ina209_deactivatePins(const channel_t *const channel, uint16_t mask){
	return ina209_clearBitsOnRegister(deviceAddress,INA209_STATUS_MASK,mask);
}
*/

retval_t ina209_configureOutput(const channel_t *const channel, ina209_outputPinCfg_t * cfg){
	retval_t ret = RV_ILLEGAL;

	ret = ina209_writeOnRegister(channel,INA209_STATUS_MASK, (1<<cfg->pin), (cfg->mode << cfg->pin));

	switch (cfg->pin){
		case WARNING_PIN:
			ret = ina209_writeOnRegister(channel,INA209_BUS_VOLTAGE_OVER_WARN, (1<<1), (cfg->polarity << 1));
			ret = ina209_writeOnRegister(channel,INA209_BUS_VOLTAGE_OVER_WARN, (1<<0), (cfg->latch << 0));
			break;
		case OVERLIMIT_PIN:
			ret = ina209_writeOnRegister(channel,INA209_BUS_VOLTAGE_OVER_LIMIT, (1<<1), (cfg->polarity << 1));
			ret = ina209_writeOnRegister(channel,INA209_BUS_VOLTAGE_OVER_LIMIT, (1<<0), (cfg->latch << 0));
			break;
		case CRITICAL_PIN:
			ret = ina209_writeOnRegister(channel,INA209_CRITICAL_DAC_POS, (1<<4), (cfg->polarity << 4));
			ret = ina209_writeOnRegister(channel,INA209_CRITICAL_DAC_POS, (1<<0), (cfg->latch << 0));
			break;
		case ALERT_PIN:
			ret = ina209_writeOnRegister(channel,INA209_STATUS_MASK, 0xFFE, cfg->SMBusMask);
			break;
	}

	return ret;
}


// --------------------------------------------------------------------------------------------
// Low level
// --------------------------------------------------------------------------------------------


retval_t ina209_readRegister(const channel_t *const channel, uint8_t address, uint16_t *value) {
	frame_t cmd    = DECLARE_FRAME_BYTES(address);
	frame_t answer = DECLARE_FRAME_SPACE(2);
	retval_t rv;

	rv = channel_transact(channel, &cmd, 0, &answer);
	if (RV_SUCCESS != rv) return rv;
	frame_reset_for_reading(&answer);

	return frame_get_u16(&answer, value);
}

retval_t ina209_writeRegister(const channel_t *const channel, uint8_t address, uint16_t value) {
	frame_t cmd    = DECLARE_FRAME_SPACE(3);

	(void)frame_put_u8(&cmd, address);
	(void)frame_put_u16(&cmd, value);
	frame_reset_for_reading(&cmd);
	return  channel_send(channel, &cmd);
}

retval_t ina209_writeOnRegister(const channel_t *const channel, uint8_t regAddress, uint16_t bitmask, uint16_t value){
	retval_t ret;
	uint16_t buffer;

	if (ina209_readRegister(channel, regAddress, &buffer)==RV_SUCCESS){
		buffer &= ~bitmask;
		buffer |= value;
		ret = ina209_writeRegister(channel, regAddress, buffer);
	}else{
		ret = RV_ERROR;
	}

	if (ret!=RV_SUCCESS){
		log_report(LOG_INA209, "ERROR!\r\n");
	}
	return ret;
}


retval_t ina209_setBitsOnRegister(const channel_t *const channel, uint8_t regAddress, uint16_t bitmask){
	uint16_t buffer;
	retval_t ret;

	ret = ina209_readRegister(channel,regAddress,&buffer);

	if (ret == RV_SUCCESS){
		buffer |= bitmask;
		ret = ina209_writeRegister(channel,regAddress,buffer);
	}

	return ret;
}

retval_t ina209_clearBitsOnRegister(const channel_t *const channel, uint8_t regAddress, uint16_t bitmask){
	uint16_t buffer;
	retval_t ret;

	ret = ina209_readRegister(channel,regAddress,&buffer);

	if (ret == RV_SUCCESS){
		buffer &=~ bitmask;
		ret = ina209_writeRegister(channel,regAddress,buffer);
	}

	return ret;
}

// --------------------------------------------------------------------------------------------
// Private - Conversion
// --------------------------------------------------------------------------------------------
#define clamp_val(val, min, max) ({             \
         typeof(val) __val = (val);              \
         typeof(val) __min = (min);              \
         typeof(val) __max = (max);              \
         __val = __val < __min ? __min: __val;   \
         __val > __max ? __max: __val; })

#define DIV_ROUND_CLOSEST(x, divisor)(                  \
 {                                                       \
         typeof(x) __x = x;                              \
         typeof(divisor) __d = divisor;                  \
         (((typeof(x))-1) > 0 ||                         \
          ((typeof(divisor))-1) > 0 || (__x) > 0) ?      \
                 (((__x) + ((__d) / 2)) / (__d)) :       \
                 (((__x) - ((__d) / 2)) / (__d));        \
 }                                                       \
 )

float ina209_from_reg(const uint8_t reg, const uint16_t val)
{
	float ret = 0.0;

	switch (reg) {
		case INA209_SHUNT_VOLTAGE:
		case INA209_SHUNT_VOLTAGE_POS_PEAK:
		case INA209_SHUNT_VOLTAGE_NEG_PEAK:
		case INA209_SHUNT_VOLTAGE_POS_WARN:
		case INA209_SHUNT_VOLTAGE_NEG_WARN:
			ret = val*10.00E-6f; // LSB = 10uV, this will return in volts unit
			break;

		case INA209_BUS_VOLTAGE:
		case INA209_BUS_VOLTAGE_MAX_PEAK:
		case INA209_BUS_VOLTAGE_MIN_PEAK:
		case INA209_BUS_VOLTAGE_OVER_WARN:
		case INA209_BUS_VOLTAGE_UNDER_WARN:
		case INA209_BUS_VOLTAGE_OVER_LIMIT:
		case INA209_BUS_VOLTAGE_UNDER_LIMIT:
			ret = (val >> 3) * 4.0E-3f; // LSB=4 mV, last 3 bits unused, return in Volts
			break;

		case INA209_CRITICAL_DAC_POS:
			/* LSB=1 mV, in the upper 8 bits */
			ret = val >> 8;
			break;

		case INA209_CRITICAL_DAC_NEG:
			/* LSB=1 mV, in the upper 8 bits */
			ret = -1.0 * (val >> 8);
			break;

		case INA209_POWER:
		case INA209_POWER_PEAK:
		case INA209_POWER_WARN:
		case INA209_POWER_OVER_LIMIT:
			/* LSB=20 mW. Convert to uW */
			ret = val * (20.0f * 100E-5f)*10E2f; // LSB harcoded as in current, return in mW
			break;

		case INA209_CURRENT:
			/* LSB=1 mA (selected). Is in mA */
			ret = (val * (100E-5f))*10E2f; // LSB hardcoded (configurate via cal register), return in mA
			break;
		default:
			log_report(LOG_INA209, "DEBUG: wrong register!\r\n");
			break;
	}

	return ret;
}

uint16_t ina209_to_reg(uint8_t reg, float val)
{
	uint16_t buffer;

	switch (reg) {
	case INA209_SHUNT_VOLTAGE_POS_WARN:
	case INA209_SHUNT_VOLTAGE_NEG_WARN:
		buffer = (uint16_t)(val/10E-6f);
		break;
		/* Limit to +- 320 mV, 10 uV LSB */
		//return clamp_val(val, -320, 320) * 100;

	case INA209_BUS_VOLTAGE_OVER_WARN:
	case INA209_BUS_VOLTAGE_UNDER_WARN:
	case INA209_BUS_VOLTAGE_OVER_LIMIT:
	case INA209_BUS_VOLTAGE_UNDER_LIMIT:
		buffer = ((uint16_t) (val / 4.0E-3f)) << 3;
		//return (DIV_ROUND_CLOSEST(clamp_val(val, 0, 32000), 4) << 3) | (old & 0x7);
		break;

	case INA209_CRITICAL_DAC_NEG:	// be carefull this is a negative value only
	case INA209_CRITICAL_DAC_POS:
		/*
		 * Limit to 0-255 mV, 1 mV LSB
		 *
		 * The value lives in the top 8 bits only, be careful
		 * and keep original value of other bits.
		 */
		buffer = ((uint16_t)(val/1E-3f)) << 8;
		break;

	case INA209_POWER_WARN:
	case INA209_POWER_OVER_LIMIT:
		buffer = (uint16_t) (val / ((20.0f * 10E-5f)*10E2f));
		break;
		/* 20 mW LSB */
		//return DIV_ROUND_CLOSEST(val, 20 * 1000);

	}
	return buffer;
}

