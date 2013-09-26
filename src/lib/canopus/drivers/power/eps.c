#include <canopus/types.h>
#include <canopus/frame.h>
#include <canopus/drivers/power/eps.h>
#include <canopus/drivers/channel.h>
#include <canopus/assert.h>
#include <canopus/drivers/adc.h>
#include <canopus/board/channels.h>
#include <canopus/logging.h>

#include <FreeRTOS.h>
#include <task.h>

typedef struct EPS_ADC_convertionValue_t {
	float factor;
	float offset;
} EPS_ADC_convertionValue_t;

static const EPS_ADC_convertionValue_t EPS_ADC_convertionTable[] = {
    { 1, 0 },                   /* 0 */		/* GND */
    { -0.54012, 691.3017 },     /* 1 */		/* Array2_mA */
    { -0.16295, 110.86113 },    /* 2 */		/* Array2_C  */
    { -0.03606, 35.01082 },     /* 3 */  /*CB2*/
    { -0.53886, 692.3604 },     /* 4 */  /*CB2*/
    { -0.16295, 110.86113 },    /* 5 */  /*NOT USED*/
    { -0.03538, 34.5597 },      /* 6 */  /*CB2*/
    { -0.54869, 695.4475 },     /* 7 */  /*CB2*/
    { -0.16295, 110.86113 },    /* 8 */  /*NOT USED*/
    { -0.009124, 8.723841 },    /* 9 */  /*NOT USED*/
    { -0.543119, 528.509296 },  /* 10 */ /*NOT USED*/
    { -0.16295, 110.86113 },    /* 11 */ /*NOT USED*/
    { 1, 0 },                   /* 12 */ /*GND?*/
    { -0.54414, 694.3171 },     /* 13 */ /*CB2*/
    { -0.16295, 110.86113 },    /* 14 */ /*NOT USED*/
    { 1, 0 },                   /* 15 */	/* GND */
    { 1, 0 },                   /* 16 */	/* GND */
    { -5.431194, 5285.092963 }, /* 17 */	/* BatteryBus_mA */
    { -0.16295, 111.18659 },    /* 18 */	/* Battery2_C */
    { -0.16295, 111.18659 },    /* 19 */	/* Battery1_C */
    { 1, 0 },                   /* 20 */	/* Battery_dir */
    { -8.20101, 8380.809 },     /* 21 */	/* Battery_mA */
    { -0.01185, 10.11521 },     /* 22 */ 	/* Battery_v  */
    { 1, 0 },                   /* 23 */
    { 1, 0 },                   /* 24 */
    { -5.40332, 5284.683},      /* 25 */	/* 12v_mA */
    { -5.39879, 5262.442},      /* 26 */	/* 5v_mA  */
    { -5.43512, 4623.646},      /* 27 */	/* 3v3_mA */
    { 1, 0 },                   /* 28 */
    { 1, 0 },                   /* 29 */
    { -0.16295, 110.86113 },    /* 30 */
    { -0.543119, 528.509296 },  /* 31 */
};

static const char *const EPS_ADC_name[] = {
    [EPS_ADC_ARRAY_X_v] = "X (V)",
    [EPS_ADC_ARRAY_X_MINUS_mA] = "-X (mA)",
    [EPS_ADC_ARRAY_X_MINUS_C] = "-X (C)",
    [EPS_ADC_ARRAY_X_PLUS_mA] = "+X (mA)",
    [EPS_ADC_ARRAY_X_PLUS_C] = "+X (C)",

    [EPS_ADC_ARRAY_Y_v] = "Y (V)",
    [EPS_ADC_ARRAY_Y_MINUS_mA] = "-Y (mA)",
    [EPS_ADC_ARRAY_Y_MINUS_C] = "-Y (C)",
    [EPS_ADC_ARRAY_Y_PLUS_mA] = "+Y (mA)",
    [EPS_ADC_ARRAY_Y_PLUS_C] = "+Y (C)",

    [EPS_ADC_ARRAY_Z_v] = "Z (V)",
    [EPS_ADC_ARRAY_Z_MINUS_C] = "-Z (C)",
    [EPS_ADC_ARRAY_Z_MINUS_mA] = "-Z (mA)",
    [EPS_ADC_ARRAY_Z_PLUS_mA] = "+Z (mA)",
    [EPS_ADC_ARRAY_Z_PLUS_C] = "+Z (C)",

    [EPS_ADC_BUS_BATTERY_mA] = "Bus Batt (mA)",
    [EPS_ADC_BUS_12v_mA] = "Bus 12v (mA)",
    [EPS_ADC_BUS_5v_mA] = "Bus 5v (mA)",
    [EPS_ADC_BUS_3v3_mA] = "Bus 3v3 (mA)",

    [EPS_ADC_BATTERY_C_1] = "Batt (C/1)",
    [EPS_ADC_BATTERY_C_2] = "Batt (C/2)",
    [EPS_ADC_BATTERY_DIRECTION] = "Batt (dir)",
    [EPS_ADC_BATTERY_mA] = "Batt (mA)",
    [EPS_ADC_BATTERY_v] = "Batt (V)",

    [EPS_ADC_GND_1] = "GND/0",
    [EPS_ADC_GND_2] = "GND/2",
    [EPS_ADC_GND_3] = "GND/3",
    [EPS_ADC_GND_4] = "GND/4",
    [EPS_ADC_GND_5] = "GND/5",
    [EPS_ADC_GND_6] = "GND/6",
    [EPS_ADC_GND_7] = "GND/7",
    [EPS_ADC_GND_8] = "GND/8",
};

static const char *
adc_name(eps_adc_channel_t chan)
{
    return NULL == EPS_ADC_name[chan] ? "unknown" : EPS_ADC_name[chan];
}

#define EPS_CMD_ADC						0
#define EPS_CMD_STATUS					1
#define EPS_CMD_PDM_OFF					2
#define EPS_CMD_VERSION					4
#define EPS_CMD_HEATER_FORCE_OFF		5
#define EPS_CMD_CLEAR_RESET_TIMER		7
#define EPS_CMD_SET_BUS_RESET_TIMEOUT	8
#define EPS_CMD_WATCHDOG				128

float eps_ADC_convertion(uint8_t channel, uint16_t rawValue) {
	EPS_ADC_convertionValue_t convert;

	assert(channel <= 31);
	convert = EPS_ADC_convertionTable[channel];
	return convert.offset + convert.factor * rawValue;
}

uint16_t eps_ADC_anticonvertion(uint8_t channel, float value) {
	EPS_ADC_convertionValue_t convert;

	assert(channel <= 31);
	convert = EPS_ADC_convertionTable[channel];
	return (value - convert.offset) / convert.factor;
}

retval_t eps_GetArrayCurrent(eps_array_t array_id, float *current) {
	uint8_t channel;

	switch (array_id) {
		case EPS_ARRAY_X_MINUS:
			channel = EPS_ADC_ARRAY_X_MINUS_mA;
			break;
		case EPS_ARRAY_X_PLUS:
			channel = EPS_ADC_ARRAY_X_PLUS_mA;
			break;
		case EPS_ARRAY_Y_MINUS:
			channel = EPS_ADC_ARRAY_Y_MINUS_mA;
			break;
		case EPS_ARRAY_Y_PLUS:
			channel = EPS_ADC_ARRAY_Y_PLUS_mA;
			break;
		case EPS_ARRAY_Z_MINUS:
			channel = EPS_ADC_ARRAY_Z_MINUS_mA;
			break;
		case EPS_ARRAY_Z_PLUS:
			channel = EPS_ADC_ARRAY_Z_PLUS_mA;
			break;
		default:
			return RV_NOENT;
	}

	return eps_GetADC(channel, current);
}

retval_t eps_GetArrayTemperature(eps_array_t array_id, float *temperature) {
	uint8_t channel;

	switch (array_id) {
	case EPS_ARRAY_X_MINUS:
		channel = EPS_ADC_ARRAY_X_MINUS_C;
		break;
	case EPS_ARRAY_X_PLUS:
		channel = EPS_ADC_ARRAY_X_PLUS_C;
		break;
	case EPS_ARRAY_Y_MINUS:
		channel = EPS_ADC_ARRAY_Y_MINUS_C;
		break;
	case EPS_ARRAY_Y_PLUS:
		channel = EPS_ADC_ARRAY_Y_PLUS_C;
		break;
	case EPS_ARRAY_Z_MINUS:
		channel = EPS_ADC_ARRAY_Z_MINUS_C;
		break;
	case EPS_ARRAY_Z_PLUS:
		channel = EPS_ADC_ARRAY_Z_PLUS_C;
		break;
	default:
		return RV_NOENT;
	}

	return eps_GetADC(channel, temperature);
}

retval_t eps_GetArrayVoltage(eps_array_t array_id, float *voltage) {
	uint8_t channel;

	switch (array_id)
	{
		case EPS_ARRAY_X:
			channel = EPS_ADC_ARRAY_X_v;
			break;
		case EPS_ARRAY_Y:
			channel = EPS_ADC_ARRAY_Y_v;
			break;
		case EPS_ARRAY_Z:
			channel = EPS_ADC_ARRAY_Z_v;
			break;
		default:
			return RV_NOENT;
	}

	return eps_GetADC(channel, voltage);
}

retval_t eps_GetBusCurrent(eps_bus_t bus_id, float *current) {
	eps_adc_channel_t channel;

	switch (bus_id)
	{
		case EPS_BUS_BATTERY:
			channel = EPS_ADC_BUS_BATTERY_mA;
			break;
		case EPS_BUS_3v3:
			channel = EPS_ADC_BUS_3v3_mA;
			break;
		case EPS_BUS_5v:
			channel = EPS_ADC_BUS_5v_mA;
			break;
		case EPS_BUS_12v:
			channel = EPS_ADC_BUS_12v_mA;
			break;
		default:
			return RV_NOENT;
	}

	return eps_GetADC(channel, current);
}

retval_t eps_GetBatteryCurrent(uint8_t battery_id, float *current) {
	if (1 != battery_id) return RV_NOENT;
	return eps_GetADC(EPS_ADC_BATTERY_mA, current);
}

retval_t eps_GetBatteryTemperature(uint8_t battery_id, float *temperature) {
    eps_adc_channel_t channel;

    switch (battery_id) {
    case 1:
        channel = EPS_ADC_BATTERY_C_1;
        break;
    case 2:
        channel = EPS_ADC_BATTERY_C_2;
        break;
    default:
        return RV_NOENT;
    }

    return eps_GetADC(channel, temperature);
}

retval_t eps_GetBatteryVoltage(uint8_t battery_id, float *voltage) {
	if (1 != battery_id) return RV_NOENT;
	return eps_GetADC(EPS_ADC_BATTERY_v, voltage);
}

retval_t eps_GetChargerActivity(uint8_t battery_id, eps_charger_activity_t *activity) {
	uint16_t value;
	retval_t rv;

	if (1 != battery_id) return RV_NOENT;
	rv = eps_GetADCRaw(EPS_ADC_BATTERY_DIRECTION, &value);
	if (RV_SUCCESS != rv) return rv;

	if      (value < 30) *activity = EPS_CHARGER_CHARGING;
	else if (value > 1000) *activity = EPS_CHARGER_DISCHARGING;
	else *activity = EPS_CHARGER_NEITHER_CHARGING_NOR_DISCHARGING;
	return RV_SUCCESS;
}

/*==============================================================================*/
/* Low level message interface */
/*==============================================================================*/
retval_t eps_GetADCRaw(eps_adc_channel_t channel_id, uint16_t *value) {
	frame_t cmd = DECLARE_FRAME_BYTES(EPS_CMD_ADC, channel_id);
	frame_t answer = DECLARE_FRAME_SPACE(2);
	retval_t rv;
    *value = ADC_ERROR_VALUE;

	rv = channel_transact(ch_eps, &cmd, 2, &answer); // TODO document '2' with define
    if (RV_SUCCESS != rv) {
        log_report_fmt(LOG_EPS, "eps_GetADCRaw(%d:%s) rv=%s\r\n", channel_id, adc_name(channel_id), retval_s(rv));
        return rv;
    }
	vTaskDelay(10 / portTICK_RATE_MS);
	frame_reset_for_reading(&answer);
	rv = frame_get_u16(&answer, value);
    log_report_fmt(LOG_EPS, "eps_GetADCRaw(%d:%s) u16=0x%04x\r\n", channel_id, adc_name(channel_id), *value);
    return rv;
}

retval_t eps_GetADC(eps_adc_channel_t channel_id, float *value) {
	retval_t rv;
	uint16_t rawData;

    if (channel_id >= EPS_ADC_CHANNEL_COUNT)
        return RV_ILLEGAL;

	rv = eps_GetADCRaw(channel_id, &rawData);
	if (RV_SUCCESS != rv) return rv;

    *value = eps_ADC_convertion(channel_id, rawData);

	return rv;
}

retval_t eps_GetStatus(uint16_t *status) {
	static const uint8_t cmd_data[] = {EPS_CMD_STATUS, 0};
	frame_t cmd = DECLARE_FRAME(cmd_data);
	frame_t answer = DECLARE_FRAME_SPACE(2);
	retval_t rv;
    *status = ADC_ERROR_VALUE;

	rv = channel_transact(ch_eps, &cmd, 0, &answer);
    log_report_fmt(LOG_EPS, "eps_GetStatus() %s\n", retval_s(rv));
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	return frame_get_u16(&answer, status);
}

retval_t eps_GetFirmwareVersion(uint16_t *version) {
	static const uint8_t cmd_data[] = {EPS_CMD_VERSION, 0};
	frame_t cmd = DECLARE_FRAME(cmd_data);
	frame_t answer = DECLARE_FRAME_SPACE(2);
	retval_t rv;
    *version = ADC_ERROR_VALUE;

    log_report(LOG_EPS, "eps_GetFirmwareVersion()\n");
	rv = channel_transact(ch_eps, &cmd, 0, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);
	return frame_get_u16(&answer, version);
}

retval_t eps_ResetPowerLines(eps_bus_t line_id) {
    retval_t rv;
	frame_t cmd = DECLARE_FRAME_BYTES(EPS_CMD_PDM_OFF, line_id);

    rv = channel_send(ch_eps, &cmd);
    log_report_fmt(LOG_EPS, "eps_ResetPowerLines(bus=%d) %s\n", line_id, retval_s(rv));
    return rv;
}

retval_t eps_Reset() {
	frame_t cmd = DECLARE_FRAME_BYTES(EPS_CMD_WATCHDOG, 0);

    log_report(LOG_EPS, "eps_Reset()\n");
	return channel_send(ch_eps, &cmd);
}

retval_t eps_HeaterMode(eps_heater_mode_t mode) {
	frame_t cmd = DECLARE_FRAME_SPACE(2);

	(void)frame_put_u8(&cmd, EPS_CMD_HEATER_FORCE_OFF);
	switch (mode) {
		case EPS_HEATER_OFF:
			(void)frame_put_u8(&cmd, 1);
			break;
		default:
			(void)frame_put_u8(&cmd, 0);
	}
	frame_reset_for_reading(&cmd);

    log_report_fmt(LOG_EPS, "eps_HeaterMode(mode=%d)\n", mode);
	return channel_send(ch_eps, &cmd);
}

retval_t eps_RestartWatchdogCounter() {
	frame_t cmd = DECLARE_FRAME_BYTES(EPS_CMD_CLEAR_RESET_TIMER, 0);

	return channel_send(ch_eps, &cmd);
}
retval_t eps_SetWatchdogTimeout(uint8_t timeout_minutes) {
	frame_t cmd = DECLARE_FRAME_BYTES(EPS_CMD_SET_BUS_RESET_TIMEOUT, timeout_minutes);

	return channel_send(ch_eps, &cmd);
}
