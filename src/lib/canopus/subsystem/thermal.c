#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/thermal.h>
#include <canopus/subsystem/aocs/aocs.h>
#include <canopus/board/adc.h>
#include <canopus/drivers/power/eps.h>
#include <canopus/logging.h>
#include <canopus/nvram.h>

static const uint8_t *sensors[] = {
		[THERMAL_ZONE_STARTRACKER] = (uint8_t[]){
			0},
		[THERMAL_ZONE_LOWER_1] = (uint8_t[]){
			0},
		[THERMAL_ZONE_LOWER_2] = (uint8_t[]){
			0},

};

static const thermal_limits_t *thermal_limits[SM_COUNT] = {
		[SM_SURVIVAL]  = nvram.thermal.thermal_limits_survival,
		[SM_MISSION]   = nvram.thermal.thermal_limits_mission,
		[SM_LOW_POWER] = nvram.thermal.thermal_limits_low_power,
};

#define AVERAGING_TIMES		16
retval_t THERMAL_get_temperature(thermal_sensors_e sensor, int32_t *temperature_milli_celcious) {
	retval_t rv;
	int times;
	float samples[ADC_CHANNELS];
	float average_v, average_mv;
	float temperature;
    *temperature_milli_celcious = ADC_ERROR_VALUE;

	switch (sensor) {
		/* BANK 0 at ADC channels. */
		case THERMAL_SENSOR_STRUCTURE:
		case THERMAL_SENSOR_SOLAR_Ym_OUTER:
		case THERMAL_SENSOR_CAMERA_BOARD:
		case THERMAL_SENSOR_BUS_PAYLOAD_BOARD:
		case THERMAL_SENSOR_CAMERA_HOUSING:
		case THERMAL_SENSOR_SVIP:
		case THERMAL_SENSOR_SOLAR_Xp_INNER:
		case THERMAL_SENSOR_SOLAR_Xp_OUTER:
		case THERMAL_SENSOR_RADIO:
		case THERMAL_SENSOR_OVERO:
		case THERMAL_SENSOR_TMS0:
		case THERMAL_SENSOR_TMS0_REG:
			average_v = 0;
			for (times=0;times<AVERAGING_TIMES;times++) {
				adc_get_samples(ADC_BANK_TEMP, samples);
				average_v += samples[sensor];
			}
			/* LM60 is 0 = 424mV and 6.25mV per °C */

			average_mv = average_v * 1000.f / AVERAGING_TIMES;
			*temperature_milli_celcious = ((average_mv - 424) / 6.25) * 1000.f;
			break;

		case THERMAL_SENSOR_RADIO_CPU:			/* Embedded in Li-1 */
			return RV_NOTIMPLEMENTED;
		case THERMAL_SENSOR_IMU:				/* Embedded in ADIS-16405 */
			return AOCS_get_imu_temperature(temperature_milli_celcious);
		case THERMAL_SENSOR_BATTERY:
			rv = eps_GetBatteryTemperature(2, &temperature);
			if (RV_SUCCESS != rv) return rv;
			*temperature_milli_celcious = (int)(temperature * 1000);
			break;
		default:
			return RV_ILLEGAL;
	}
	return RV_SUCCESS;
}
#undef AVERAGING_TIMES

static retval_t THERMAL_turn_heaters_on(thermal_zones_t zone) {
	log_report(LOG_SS_THERMAL, "Turning heaters on\n");
	switch (zone) {
		case THERMAL_ZONE_STARTRACKER:
			return POWER_turnOn(POWER_SW_HEATER_1);
		case THERMAL_ZONE_LOWER_1:
			return POWER_turnOn(POWER_SW_HEATER_2);
		case THERMAL_ZONE_LOWER_2:
			return POWER_turnOn(POWER_SW_HEATER_3);
		case THERMAL_ZONE_COUNT:
		default:
			return RV_ILLEGAL;
	}
}

static retval_t THERMAL_turn_heaters_off(thermal_zones_t zone) {
	log_report(LOG_SS_THERMAL, "Turning heaters on\n");
	switch (zone) {
		case THERMAL_ZONE_STARTRACKER:
			return POWER_turnOff(POWER_SW_HEATER_1);
		case THERMAL_ZONE_LOWER_1:
			return POWER_turnOff(POWER_SW_HEATER_2);
		case THERMAL_ZONE_LOWER_2:
			return POWER_turnOff(POWER_SW_HEATER_3);
		case THERMAL_ZONE_COUNT:
			break;
	}
	return RV_ILLEGAL;
}

static retval_t compute_zone_average(satellite_mode_e mode, int32_t *zone_average) {
	thermal_zones_t zone;
	int32_t temperature;
	retval_t rv;
	int i, sensor_count;

	for (zone = 0;zone < THERMAL_ZONE_COUNT; zone++) {
		zone_average[zone] = 0;
		sensor_count = 0;
		for (i=0; sensors[zone][i]; i++) {
			rv = THERMAL_get_temperature(sensors[zone][i], &temperature);
			if (RV_SUCCESS != rv) return rv;
			zone_average[zone] += temperature;
			sensor_count++;
		}
		if (0 == sensor_count) continue;
		zone_average[zone] /= sensor_count;
	}
	return RV_SUCCESS;
}

static void turn_on_off_as_limits(satellite_mode_e mode, int32_t *zone_average) {
	const thermal_limits_t *limits = thermal_limits[mode];
	thermal_zones_t zone;

	for (zone = 0;zone < THERMAL_ZONE_COUNT; zone++) {
		if (zone_average[zone] < limits[zone].turn_on_heaters_temperature) {
			THERMAL_turn_heaters_on(zone);
		}
		if (zone_average[zone] > limits[zone].turn_off_heaters_temperature) {
			THERMAL_turn_heaters_off(zone);
		}
	}
}

static void thermal_tick(satellite_mode_e mode) {
	int32_t zone_average[THERMAL_ZONE_COUNT];
	uint32_t matrix_disable_key;

	matrix_disable_key = nvram.thermal.matrix_disable_key;

	FUTURE_HOOK_3(thermal_kick, &mode, zone_average, &matrix_disable_key);
	if (matrix_disable_key == MATRIX_KEY_DISABLED) return;
	if (RV_SUCCESS == compute_zone_average(mode, zone_average)) {
		turn_on_off_as_limits(mode, zone_average);
	}
}

static void THERMAL_main_task(void *pvParameters) {
	satellite_mode_e mode, prev_mode;
    subsystem_t * ss = (subsystem_t *)pvParameters;

	PLATFORM_ss_is_ready(ss);

    mode = PLATFORM_current_satellite_mode();
    while (1) {
		thermal_tick(mode);
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode, PM_MODE_CHANGE_DETECT_TIME_SLOT_MS / portTICK_RATE_MS)) {
			/* It's the same in all modes.
			 * POWER will disable the switched lines when thermal is not allowed to act */
			prev_mode = PLATFORM_previous_satellite_mode();
			FUTURE_HOOK_3(thermal_mode_change, ss, &prev_mode, &mode);
			PLATFORM_ss_is_ready(ss);
		}
		PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);
	}
}

#define GET_PUT_TEMPERATURE(SENSOR) get_put_temperature(SENSOR, oframe, &temperature)
static inline retval_t get_put_temperature(thermal_sensors_e sensor, frame_t * oframe, int32_t *temperature) {
	*temperature = ADC_ERROR_VALUE;

    if (RV_SUCCESS == THERMAL_get_temperature(sensor, temperature)) {
            *temperature /= 10;
    }
    return frame_put_s16(oframe, *temperature);
}

static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	int32_t temperature;
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_BATTERY);
	return GET_PUT_TEMPERATURE(THERMAL_SENSOR_RADIO);
    /* FIXME weird retval */
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	int32_t temperature;
    if (RV_SUCCESS == THERMAL_get_temperature(THERMAL_SENSOR_BATTERY, &temperature)) {
            temperature /= 1000;
    } else {
    	temperature = -127;
    }
    frame_put_bits_by_4(oframe, temperature, 8);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	char buf[20];
	int32_t temp_mC;
	float temp_C;
	int n;

	THERMAL_get_temperature(THERMAL_SENSOR_BATTERY, &temp_mC);
	temp_C = temp_mC/1000.;
	n = snprintf(buf, sizeof(buf), "Temp:%4.1fC ", temp_C);
	if (n < 0) {
		return RV_ERROR;
	}

    return frame_put_data(oframe, buf, n);
}

static retval_t cmd_set_matrix_key(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return frame_get_u32(iframe, &nvram.thermal.matrix_disable_key);
}

static retval_t cmd_get_status(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	int32_t temperature; /* FIXME why 32bit? */

	GET_PUT_TEMPERATURE(THERMAL_SENSOR_STRUCTURE);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_SOLAR_Ym_OUTER);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_CAMERA_BOARD);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_BUS_PAYLOAD_BOARD);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_CAMERA_HOUSING);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_SVIP);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_SOLAR_Xp_INNER);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_SOLAR_Xp_OUTER);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_RADIO);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_BATTERY);

//	GET_PUT_TEMPERATURE(THERMAL_SENSOR_RADIO_CPU);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_IMU);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_OVERO);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_TMS0);
	GET_PUT_TEMPERATURE(THERMAL_SENSOR_TMS0_REG);
	return frame_put_u32(oframe, nvram.thermal.matrix_disable_key);
}

const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("battery_C:s16:[battery_C / 100.0], radio_temp:s16:[radio_temp/100.0]", "battery_temp:u8"),
	DECLARE_COMMAND(SS_CMD_THERMAL_SET_MATRIX_KEY, cmd_set_matrix_key, "setMatrix", "Set Thermal matrix disable key <secret key>", "key:u32", ""),
	DECLARE_COMMAND(SS_CMD_THERMAL_GET_STATUS, cmd_get_status, "status", "Return values for all temperature sensors", "", "structure_C:s16:[structure_C/100.0], panel_Ym_Outer_C:s16:[panel_Ym_Outer_C/100.0], camera_board_C:s16:[camera_board_C/100.0], Electronics_C:s16:[Electronics_C/100.0], camera_housing_C:s16:[camera_housing_C/100.0], SVIP_C:s16:[SVIP_C/100.0], panel_Xp_Inner_C:s16:[panel_Xp_Inner_C/100.0], panel_Xp_Outer_C:s16:[panel_Xp_Outer_C/100.0], radio_C:s16:[radio_C/100.0], battery_C:s16:[battery_C/100.0], IMU_C:s16:[IMU_C/100.0], Overo_C:s16:[Overo_C/100.0], TMS_C:s16:[TMS_C/100.0], TMS_Reg_C:s16:[TMS_Reg_C/100.0], matrixKey:u32"),
};

static subsystem_api_t subsystem_api = {
    .main_task = &THERMAL_main_task,
    .command_execute = &ss_command_execute,
};

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_THERMAL,
    .usStackDepth = STACK_DEPTH_THERMAL,
    .id = SS_THERMAL,
    .name = "THERMAL",
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_THERMAL = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = &subsystem_state
};
