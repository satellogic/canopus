/* This header file contains the API for the Electronic Power System */
/* For CubeBug-1                                                     */
/* Based on CS-1UEPS2-NB as of 21/07/2010 (Mostly chapter 11)        */
/*                                                                   */
/* The EPS provides telemetry: current, temperature and voltage on   */
/* the panels, general status and version of the board.              */
/* It also exposes a few cotrol commands, to force the battery       */
/* heater off, to pulse off any of the output voltage lines, and to  */
/* reset the I2C node back into a default state.                     */
/* All this functionality must be exposed by the driver's API        */
/*                                                                   */
/* I2C Id: 0x2d                                                      */
/* Max Speed: 400 Kbit                                               */
/* Typical Speed: 100 Kbit                                           */

#ifndef __DRIVERS_EPS_H__
#define __DRIVERS_EPS_H__

#define EPS_API_VERSION	1

/* note: GetStatus() retrieves the status from the EPS, the result   */
/* could be expressed in a struct with bitfields, with enums or with */
/* defines. Bitfields seem a good option, however, there are groups  */
/* of bits, hence we may want to do things like check several bits   */
/* at the same time, to know if there was a protocol or an i2c error */

#define EPS_BATTERY_CRITICAL_LEVEL	(6.4f)
#define EPS_FIRMWARE_VERSION 0x3363
#define EPS_INTERNAL_R_OHM			0.40f
#define IS_VALID_ADC_VALUE(_value_)		(((_value_) != 0) && (((_value_) & ~0x03FF) == 0))

typedef enum eps_adc_channel_e {
	EPS_ADC_GND_1 = 0,

	EPS_ADC_ARRAY_X_PLUS_mA = 1,	/* Array 2 */
	EPS_ADC_ARRAY_X_PLUS_C,
	EPS_ADC_ARRAY_X_v,
	EPS_ADC_ARRAY_X_MINUS_mA,
	EPS_ADC_ARRAY_X_MINUS_C,

	EPS_ADC_ARRAY_Y_v = 6,			/* Array 1 */
	EPS_ADC_ARRAY_Y_MINUS_mA,
	EPS_ADC_ARRAY_Y_MINUS_C,

	EPS_ADC_ARRAY_Z_v = 9,			/* Array 3 */
	EPS_ADC_ARRAY_Z_PLUS_mA,
	EPS_ADC_ARRAY_Z_PLUS_C,

	EPS_ADC_GND_2 = 12,

	EPS_ADC_ARRAY_Y_PLUS_mA = 13,
	EPS_ADC_ARRAY_Y_PLUS_C,

	EPS_ADC_GND_3 = 15,
	EPS_ADC_GND_4,

	EPS_ADC_BUS_BATTERY_mA = 17,

	EPS_ADC_BATTERY_C_2 = 18,
	EPS_ADC_BATTERY_C_1,
	EPS_ADC_BATTERY_DIRECTION,
	EPS_ADC_BATTERY_mA,
	EPS_ADC_BATTERY_v,

	EPS_ADC_GND_5 = 23,
	EPS_ADC_GND_6,

	EPS_ADC_BUS_12v_mA = 25,
	EPS_ADC_BUS_5v_mA,
	EPS_ADC_BUS_3v3_mA,

	EPS_ADC_GND_7 = 28,
	EPS_ADC_GND_8,

	EPS_ADC_ARRAY_Z_MINUS_C = 30,
	EPS_ADC_ARRAY_Z_MINUS_mA,

    EPS_ADC_CHANNEL_COUNT /* MAX */
} eps_adc_channel_t;

typedef enum eps_array_e {
	EPS_ARRAY_X_PLUS,
	EPS_ARRAY_X_MINUS,
	EPS_ARRAY_Y_PLUS,
	EPS_ARRAY_Y_MINUS,
	EPS_ARRAY_Z_PLUS,
	EPS_ARRAY_Z_MINUS,
	EPS_ARRAY_X,
	EPS_ARRAY_Y,
	EPS_ARRAY_Z,
} eps_array_t;

typedef enum eps_bus_e {
   EPS_BUS_BATTERY	= 1,
   EPS_BUS_5v		= 2,
   EPS_BUS_3v3		= 4,
   EPS_BUS_12v		= 8
} eps_bus_t;

typedef enum eps_heater_mode_e {
	EPS_HEATER_OFF,
	EPS_HEATER_AUTO
} eps_heater_mode_t;

typedef enum eps_status_e {
/* This enum is in the edge to break the 16 bits limit
 * However, since this numbers are defined in the datasheet
 * they should not change unnoticed. Be careful if the go
 * past 16 bits */
	EPS_STATUS_I2C_ERROR			= 0x0001,
	EPS_STATUS_I2C_COLLISION		= 0x0002,
	EPS_STATUS_I2C_OVERFLOW			= 0x0004,
	EPS_STATUS_MSG_TOO_LONG			= 0x0008,
	EPS_STATUS_BAD_COMMAND			= 0x0100,
	EPS_STATUS_BAD_ARGUMENT			= 0x0200,
	EPS_STATUS_ADC_NOT_READY		= 0x0400,
	EPS_STATUS_OSCILATOR_RUNNING	= 0x1000,
	EPS_STATUS_WATCHDOG_RESET		= 0x2000,
	EPS_STATUS_NO_POWER_ON_RESET	= 0x4000,
	EPS_STATUS_NO_BROWN_OUT_RESET	= 0x8000
} eps_status_t;

typedef enum eps_charger_activity_e {
	EPS_CHARGER_CHARGING,
	EPS_CHARGER_DISCHARGING,
	EPS_CHARGER_NEITHER_CHARGING_NOR_DISCHARGING,
} eps_charger_activity_t;

/* ADC channels High level */
retval_t eps_GetArrayCurrent(eps_array_t array_id, float *current);         /* only on X+,X-,Z+,Z-,Y+,Y- */
retval_t eps_GetArrayTemperature(eps_array_t array_id, float *temperature); /* only on X+,X-,Z+,Z-,Y+,Y- */
retval_t eps_GetArrayVoltage(eps_array_t array_id, float *voltage);         /* only on X, Y, Z */
retval_t eps_GetBusCurrent(eps_bus_t bus_id, float *current);
retval_t eps_GetBatteryCurrent(uint8_t battery_id, float *current);
retval_t eps_GetBatteryTemperature(uint8_t battery_id, float *temperature);
retval_t eps_GetBatteryVoltage(uint8_t battery_id, float *voltage);
retval_t eps_GetChargerActivity(uint8_t battery_id, eps_charger_activity_t *activity);

 /* Low level message interface */
retval_t eps_GetADCRaw(eps_adc_channel_t channel_id, uint16_t *value);
retval_t eps_GetADC(eps_adc_channel_t channel_id, float *value);
retval_t eps_GetStatus(uint16_t *status);
retval_t eps_GetFirmwareVersion(uint16_t *version);
retval_t eps_ResetPowerLines(eps_bus_t line_id);
retval_t eps_Reset(void);
retval_t eps_HeaterMode(eps_heater_mode_t mode);
retval_t eps_RestartWatchdogCounter();
retval_t eps_SetWatchdogTimeout(uint8_t timeout_minutes);
float eps_ADC_convertion(uint8_t channel, uint16_t rawValue);
uint16_t eps_ADC_anticonvertion(uint8_t channel, float value);
#endif	/* __DRIVERS_EPS_H__ */

