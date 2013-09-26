#include <canopus/assert.h>
#include <canopus/logging.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/power.h>
#include <canopus/subsystem/platform.h>
#include <canopus/drivers/adc.h>
#include <canopus/drivers/i2c.h>
#include <canopus/drivers/power/eps.h>
#include <canopus/nvram.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/power/shifted_switches.h>
#include <canopus/drivers/power/ina209_powerdomain.h>
#include <canopus/drivers/commhub_1500.h>

typedef enum switch_config_e {
		SW____0 = 0,
		SW__1__ = 1,
		SW_FREE,		/* anybody can decide to switch on or off the line, POWER will switch it off when entering the mode */
} switch_config_e;

/* Column indexes must be in synch with satellite_mode_e in subsystem.h */
static const switch_config_e
power_matrix[][SM_COUNT] = {              		/* OFF    BOOTING 	 INITIALIZING   SURVIVAL   MISSION  LOW_POWER */
	[POWER_SW_OVERO]						= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_TMS_OTHER]					= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW____0 , SW____0 },
	[POWER_SW_IMU]							= { SW____0 , SW____0	   , SW__1__ ,  SW__1__  , SW__1__ , SW_FREE },
	[POWER_SW_HEATER_1]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_2]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_3]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_4]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_5]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_6]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_7]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_HEATER_8]						= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW____0 },
	[POWER_SW_ANTENNA_DEPLOY_1]				= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW_FREE },
	[POWER_SW_ANTENNA_DEPLOY_2]				= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW_FREE },
	[POWER_SW_ANTENNA_DEPLOY_3]				= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW_FREE },
	[POWER_SW_ANTENNA_DEPLOY_4]				= { SW____0 , SW____0	   , SW____0 ,  SW_FREE  , SW_FREE , SW_FREE },
	[POWER_SW_PD_TESTER_A]					= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_PD_TESTER_B]					= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_PD_TESTER_C]					= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_PD_TESTER_D]					= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_SVIP]							= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_CONDOR]						= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_PD_3v3]						= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
	[POWER_SW_PD_5v]						= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW____0 , SW____0 },
	[POWER_SW_PD_12v]						= { SW____0 , SW____0	   , SW____0 ,  SW____0  , SW_FREE , SW____0 },
};

static const uint16_t switches_mapping[] = {
		[POWER_SW_IMU]							= 1 << 4,	// FPGA_GPIO_MASK_IMU
		[POWER_SW_SVIP]							= 1 << 12,	// FPGA_GPIO_MASK_SVIP
		[POWER_SW_CONDOR]						= 1 << 10,	// FPGA_GPIO_MASK_CONDOR
		[POWER_SW_SDR]							= 1 << 13,  // FPGA_GPIO_MASK_SDR
		[POWER_SW_GPS]							= 1 << 14,  // FPGA_GPIO_MASK_GPS
		[POWER_SW_HEATER_1]						= SHIFTED_SW_HEATER_1,
		[POWER_SW_HEATER_2]						= SHIFTED_SW_HEATER_2,
		[POWER_SW_HEATER_3]						= SHIFTED_SW_HEATER_3,
		[POWER_SW_HEATER_4]						= SHIFTED_SW_HEATER_4,
		[POWER_SW_HEATER_5]						= SHIFTED_SW_HEATER_5,
		[POWER_SW_HEATER_6]						= SHIFTED_SW_HEATER_6,
		[POWER_SW_HEATER_7]						= SHIFTED_SW_HEATER_7,
		[POWER_SW_HEATER_8]						= SHIFTED_SW_HEATER_8,
		[POWER_SW_ANTENNA_DEPLOY_1]				= SHIFTED_SW_ANT_DPL_1,
		[POWER_SW_ANTENNA_DEPLOY_2]				= SHIFTED_SW_ANT_DPL_2,
		[POWER_SW_ANTENNA_DEPLOY_3]				= SHIFTED_SW_ANT_DPL_3,
		[POWER_SW_ANTENNA_DEPLOY_4]				= SHIFTED_SW_ANT_DPL_4,
		[POWER_SW_PD_TESTER_A]					= SHIFTED_SW_PD_TEST_A,
		[POWER_SW_PD_TESTER_B]					= SHIFTED_SW_PD_TEST_B,
		[POWER_SW_PD_TESTER_C]					= SHIFTED_SW_PD_TEST_C,
		[POWER_SW_PD_TESTER_D]					= SHIFTED_SW_PD_TEST_D,
};
uint16_t eps_adc_raw[EPS_ADC_CHANNEL_COUNT];
uint16_t pd_3v3_status,pd_5v_status,pd_12v_status;
static float improved_battery_v = 0.0f;
float pd_3v3_mV, pd_3v3_mA;
float pd_5v_mV, pd_5v_mA;
float pd_12v_mV, pd_12v_mA;
uint16_t cumulative_eps_error_count;
bool eps_broken;

switch_config_e currently_desired_switch_state(power_switch_e);

static bool eps_is_interesting_adc_channel(eps_adc_channel_t channel) {
	switch (channel) {
		case EPS_ADC_ARRAY_X_v:
		case EPS_ADC_ARRAY_X_MINUS_mA:
		case EPS_ADC_ARRAY_X_PLUS_mA:
		case EPS_ADC_ARRAY_Y_v:
		case EPS_ADC_ARRAY_Y_MINUS_mA:
		case EPS_ADC_ARRAY_Y_PLUS_mA:
		case EPS_ADC_ARRAY_Z_v:
		case EPS_ADC_ARRAY_Z_MINUS_mA:
		case EPS_ADC_ARRAY_Z_PLUS_mA:
		case EPS_ADC_BATTERY_v:
		case EPS_ADC_BATTERY_mA:
		case EPS_ADC_BATTERY_C_1:
		case EPS_ADC_BATTERY_DIRECTION:
		case EPS_ADC_BUS_BATTERY_mA:
		case EPS_ADC_BUS_3v3_mA:
		case EPS_ADC_BUS_5v_mA:
		case EPS_ADC_BUS_12v_mA:
			return true;

		case EPS_ADC_GND_1:
		case EPS_ADC_GND_2:
		case EPS_ADC_GND_3:
		case EPS_ADC_GND_4:
		case EPS_ADC_GND_5:
		case EPS_ADC_GND_6:
		case EPS_ADC_GND_7:
		case EPS_ADC_GND_8:
		case EPS_ADC_ARRAY_X_MINUS_C:
		case EPS_ADC_ARRAY_X_PLUS_C:
		case EPS_ADC_ARRAY_Y_MINUS_C:
		case EPS_ADC_ARRAY_Y_PLUS_C:
		case EPS_ADC_ARRAY_Z_MINUS_C:
		case EPS_ADC_ARRAY_Z_PLUS_C:
		case EPS_ADC_BATTERY_C_2:
		case EPS_ADC_CHANNEL_COUNT:
			break;
	}
	return false;
}

retval_t POWER_get_battery_voltage(float *voltage) {
	if (improved_battery_v == 0.0f) return RV_ERROR;
	*voltage = improved_battery_v;
	return RV_SUCCESS;
}

#define ADC_INVALID_VALUE	0xffff
#define FILTERING_FACTOR	0.25f

static void improve_battery_voltage_measurement() {
	float battery_v, battery_mA;

	if (!IS_VALID_ADC_VALUE(eps_adc_raw[EPS_ADC_BATTERY_v])) return;
	if (!IS_VALID_ADC_VALUE(eps_adc_raw[EPS_ADC_BATTERY_mA])) return;

	/* Recover real battery voltage pondering in the internal R of the battery */
	battery_v  = eps_ADC_convertion(EPS_ADC_BATTERY_v,  eps_adc_raw[EPS_ADC_BATTERY_v]);
	battery_mA = eps_ADC_convertion(EPS_ADC_BATTERY_mA, eps_adc_raw[EPS_ADC_BATTERY_mA]);
	battery_v  = battery_v + EPS_INTERNAL_R_OHM * battery_mA / 1000.0f;

	if (improved_battery_v == 0.0f) {
		/* Take first measurement to start */
		improved_battery_v = battery_v;
	} else {
		/* Do some exponential smoothing with a factor of 1/10th */
		improved_battery_v *= 1.0f - FILTERING_FACTOR;
		improved_battery_v += battery_v * FILTERING_FACTOR;
	}
}

static void update_raw_telemetry(bool force_all_channels) {
	eps_adc_channel_t channel_id;
	float new_value;
	retval_t rv;

    eps_broken = false;
	for (channel_id=0;channel_id<ARRAY_COUNT(eps_adc_raw);channel_id++) {
		if (force_all_channels || eps_is_interesting_adc_channel(channel_id))
			if (RV_SUCCESS != eps_GetADCRaw(channel_id, &eps_adc_raw[channel_id])) {
				eps_adc_raw[channel_id] = ADC_INVALID_VALUE;
				eps_broken = true;
				cumulative_eps_error_count++;
			}
	    }

	improve_battery_voltage_measurement();

	pd_3v3_status = 0xFFFF;
	(void)ina209_getStatus(ch_ina_pd3v3, &pd_3v3_status);
	rv = ina209_getBusVoltage(ch_ina_pd3v3, &new_value);
	if (RV_SUCCESS == rv) pd_3v3_mV = new_value * 1000.0;

	(void)ina209_getCurrent(ch_ina_pd3v3, &pd_3v3_mA);

	pd_5v_status = 0xFFFF;
	(void)ina209_getStatus(ch_ina_pd5v, &pd_5v_status);
	rv = ina209_getBusVoltage(ch_ina_pd5v, &new_value);
	if (RV_SUCCESS == rv) pd_5v_mV = new_value * 1000.0;

	(void)ina209_getCurrent(ch_ina_pd5v, &pd_5v_mA);

	pd_12v_status = 0xFFFF;
	(void)ina209_getStatus(ch_ina_pd12v, &pd_12v_status);
	rv = ina209_getBusVoltage(ch_ina_pd12v, &new_value);
	if (RV_SUCCESS == rv) pd_12v_mV = new_value * 1000.0;

	(void)ina209_getCurrent(ch_ina_pd12v, &pd_12v_mA);
}

static void change_mode_according_to_battery_v(subsystem_t *ss) {
	float battery_v;
	retval_t rv;
	ss_power_state_t *ss_state = (ss_power_state_t*)ss->state;
	satellite_mode_e current_mode;

	rv = POWER_get_battery_voltage(&battery_v);
	if (RV_SUCCESS != rv) return;

	current_mode = PLATFORM_current_satellite_mode();

	if (battery_v <= EPS_BATTERY_CRITICAL_LEVEL) {
		PLATFORM_save_boot_reason("Battery critical");
	}
	if (battery_v <= nvram.power.battery_low_voltage_v) {
		if (current_mode != SM_LOW_POWER) {
			ss_state->low_voltage_counter++;
			PLATFORM_request_mode_change(SM_LOW_POWER);
		}
	}
	if (battery_v >= nvram.power.battery_ok_voltage_v) {
		if (current_mode == SM_LOW_POWER) {
			PLATFORM_save_boot_reason_unexpected();
			PLATFORM_request_mode_change(SM_SURVIVAL);
		}
	}
	if (battery_v >= nvram.power.battery_mission_voltage_v) {
		if (current_mode == SM_SURVIVAL) {
			PLATFORM_request_mode_change(SM_MISSION);
		}
	}
}

switch_config_e currently_desired_switch_state(power_switch_e switch_id) {
	satellite_mode_e current_mode;
	switch_config_e  desired_state;

	if (MATRIX_KEY_DISABLED == nvram.power.matrix_disable_key) return SW_FREE;

	current_mode = PLATFORM_current_satellite_mode();
	desired_state = power_matrix[switch_id][current_mode];

	if (POWER_SW_IMU == switch_id) {
		if (nvram.power.imu_power_forced_off) desired_state = SW____0;
	}

	FUTURE_HOOK_3(currently_desired_switch_state, &desired_state, switch_id, current_mode);
	return desired_state;
}

retval_t POWER_watchdog_restart_counter() {
	return eps_RestartWatchdogCounter();
}
retval_t POWER_reset() {
	return eps_ResetPowerLines(EPS_BUS_BATTERY | EPS_BUS_3v3 | EPS_BUS_5v | EPS_BUS_12v);
}
retval_t POWER_set_all_shifted_switches(uint16_t switches) {
	return shifted_setAll(switches);
}
retval_t POWER_turnOn(power_switch_e switch_id) {
	retval_t rv;

	if (currently_desired_switch_state(switch_id) == SW____0) {
			rv = RV_ILLEGAL;
	} else {
		switch (switch_id) {
			case POWER_SW_OVERO:
				rv = commhub_andRegister(COMMHUB_REG_POWER, ~COMMHUB_DEV_POWER_OVERO);
				break;
			case POWER_SW_HEATER_1:
			case POWER_SW_HEATER_2:
			case POWER_SW_HEATER_3:
			case POWER_SW_HEATER_4:
			case POWER_SW_HEATER_5:
			case POWER_SW_HEATER_6:
			case POWER_SW_HEATER_7:
			case POWER_SW_HEATER_8:
			case POWER_SW_ANTENNA_DEPLOY_1:
			case POWER_SW_ANTENNA_DEPLOY_2:
			case POWER_SW_ANTENNA_DEPLOY_3:
			case POWER_SW_ANTENNA_DEPLOY_4:
			case POWER_SW_PD_TESTER_A:
			case POWER_SW_PD_TESTER_B:
			case POWER_SW_PD_TESTER_C:
			case POWER_SW_PD_TESTER_D:
				rv = shifted_turnOn(switches_mapping[switch_id]);
				break;
			case POWER_SW_PD_3v3:
				rv = PD_ina_turnOn(ch_ina_pd3v3);
				break;
			case POWER_SW_PD_5v:
				rv = PD_ina_turnOn(ch_ina_pd5v);
				break;
			case POWER_SW_PD_12v:
				rv = PD_ina_turnOn(ch_ina_pd12v);
				break;
			case POWER_SW_IMU:
			case POWER_SW_SVIP:
			case POWER_SW_CONDOR:
				rv = commhub_orRegister(COMMHUB_REG_GP_OUT, switches_mapping[switch_id]);
				break;
			case POWER_SW_GPS:
			case POWER_SW_SDR:
			case POWER_SW_COUNT:
			case POWER_SW_TMS_OTHER:
				rv = RV_SUCCESS;
				break;
		}
	}

	return rv;
}

retval_t POWER_turnOff(power_switch_e switch_id) {
	retval_t rv;

	if (currently_desired_switch_state(switch_id) == SW__1__) {
		rv = RV_ILLEGAL;
	} else {
		switch (switch_id) {
			case POWER_SW_OVERO:
				rv = commhub_orRegister(COMMHUB_REG_POWER, COMMHUB_DEV_POWER_OVERO);
				break;
			case POWER_SW_HEATER_1:
			case POWER_SW_HEATER_2:
			case POWER_SW_HEATER_3:
			case POWER_SW_HEATER_4:
			case POWER_SW_HEATER_5:
			case POWER_SW_HEATER_6:
			case POWER_SW_HEATER_7:
			case POWER_SW_HEATER_8:
			case POWER_SW_ANTENNA_DEPLOY_1:
			case POWER_SW_ANTENNA_DEPLOY_2:
			case POWER_SW_ANTENNA_DEPLOY_3:
			case POWER_SW_ANTENNA_DEPLOY_4:
			case POWER_SW_PD_TESTER_A:
			case POWER_SW_PD_TESTER_B:
			case POWER_SW_PD_TESTER_C:
			case POWER_SW_PD_TESTER_D:
				rv = shifted_turnOff(switches_mapping[switch_id]);
				break;
			case POWER_SW_PD_3v3:
				rv = PD_ina_turnOff(ch_ina_pd3v3);
				break;
			case POWER_SW_PD_5v:
				rv = PD_ina_turnOff(ch_ina_pd5v);
				break;
			case POWER_SW_PD_12v:
				rv = PD_ina_turnOff(ch_ina_pd12v);
				break;
			case POWER_SW_IMU:
			case POWER_SW_SVIP:
			case POWER_SW_CONDOR:
				rv = commhub_andRegister(COMMHUB_REG_GP_OUT, ~switches_mapping[switch_id]);
				break;
			case POWER_SW_GPS:
			case POWER_SW_SDR:
			case POWER_SW_COUNT:
			case POWER_SW_TMS_OTHER:
				rv = RV_SUCCESS;
				break;
		}
	}

	return rv;
}

static void POWER_telemetry_update_task(void *_ss) {
	uint32_t seconds;
	portTickType last_update_time;

	last_update_time = xTaskGetTickCount();
	seconds = 0;
	while (1) {
		if (0 == (seconds % nvram.power.telemetry_update_time_s)) {
			update_raw_telemetry(false);
			change_mode_according_to_battery_v(_ss);
		}
		vTaskDelayUntil(&last_update_time, 1000 / portTICK_RATE_MS);
		seconds++;
	}
}

#define TELEMETRY_UPDATE_STACKSIZE 1024
static retval_t create_update_telemetry_task(subsystem_t *ss) {
    subsystem_config_t* ss_config = ss->config;
    xTaskHandle update_telemtry_task;
    portBASE_TYPE rv;

    rv = xTaskCreate(
    		&POWER_telemetry_update_task,
            (signed char*)"POWER/update_telemetry",
            TELEMETRY_UPDATE_STACKSIZE,
    		(void*)ss,
    		ss_config->uxPriority,
    		&update_telemtry_task);

    if (pdTRUE != rv) {
        log_report(LOG_SS_POWER, "Couldn't create POWER telemetry update task\n");
    	return RV_ERROR;
    }
    return RV_SUCCESS;
}

static retval_t power_on_and_off_as_matrix(subsystem_t *ss) {
	power_switch_e switch_id;
	switch_config_e wanted_state;
	bool success = true;
	retval_t rv;

	if (nvram.power.matrix_disable_key == MATRIX_KEY_DISABLED) return RV_SUCCESS;

	for (switch_id = 0; switch_id < POWER_SW_COUNT; switch_id++) {

		wanted_state = currently_desired_switch_state(switch_id);
		switch (wanted_state) {
			case SW____0:
				rv = POWER_turnOff(switch_id);
				success &= (RV_SUCCESS == rv);
				break;
			case SW__1__:
				rv = POWER_turnOn(switch_id);
				success &= (RV_SUCCESS == rv);
				break;
			case SW_FREE:
				break;
		}
	}

	return success? RV_SUCCESS : RV_ERROR;
}

static void initialize_channels() {
	retval_t rv;

    rv = channel_open(ch_eps);
    if (RV_SUCCESS != rv);

    rv = channel_open(ch_ina_pd3v3);
    if (RV_SUCCESS != rv);

    rv = channel_open(ch_ina_pd5v);
    if (RV_SUCCESS != rv);

    rv = channel_open(ch_ina_pd12v);
    if (RV_SUCCESS != rv);
}

static void initialie_devices() {
	shifter_initialize();
	PD_ina_initialize(ch_ina_pd3v3, 3.3f, 1000.f);
	PD_ina_initialize(ch_ina_pd5v,  5.f,  1000.f);
	PD_ina_initialize(ch_ina_pd12v, 12.f, 20000.f);
}

static void initialize_variables() {
	cumulative_eps_error_count = 0;
}

static void POWER_main_task(void *pvParameters) {
	satellite_mode_e mode = SM_BOOTING;
	satellite_mode_e prev_mode;

    subsystem_t *ss = (subsystem_t *)pvParameters;
    ss_power_state_t *ss_state = (ss_power_state_t *)ss->state;

	PLATFORM_ss_is_ready(ss);
    ss_state->wanted_switches_state = 0;

    while (1) {
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode, PM_MODE_CHANGE_DETECT_TIME_SLOT_MS / portTICK_RATE_MS)) {
			prev_mode = PLATFORM_previous_satellite_mode();
			FUTURE_HOOK_3(power_mode_change, ss, &prev_mode, &mode);

			if (prev_mode == SM_BOOTING && mode == SM_INITIALIZING) {
				initialize_channels();
				initialie_devices();
				initialize_variables();
				create_update_telemetry_task(ss);
				PLATFORM_ss_is_ready(ss);
			}
			if (RV_SUCCESS == power_on_and_off_as_matrix(ss)) {
				PLATFORM_ss_is_ready(ss);
			}
		} else {
			// No mode change. Tick
			PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);

			switch (mode) {
			case SM_OFF:
			case SM_BOOTING:
			case SM_INITIALIZING:
			case SM_SURVIVAL:
			case SM_MISSION:
			case SM_LOW_POWER:
				/* Will not enforce it all the time, only on mode changes,
				 * this virtually makes all as FREE, the operator can turn
				 * on and off as desired
				 */
				/* power_on_and_off_as_matrix(ss, mode); */
			case SM_COUNT:
				break;
			}
		}
	}
}

/* COMMANDS */
static retval_t cmd_get_telemetry_beacon(const subsystem_t *ss, frame_t * iframe, frame_t * oframe) {
	ss_power_state_t *ss_state = (ss_power_state_t*)ss->state;
	float voltage = 0.0f;

	(void /*0.0f if error*/)POWER_get_battery_voltage(&voltage);

	frame_put_u16(oframe, ss_state->low_voltage_counter);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_BATTERY_v]);
	frame_put_u16(oframe, voltage * 1000);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_BATTERY_mA]);

	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_ARRAY_X_v]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_ARRAY_X_PLUS_mA]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_ARRAY_X_MINUS_mA]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_ARRAY_Y_v]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_ARRAY_Y_PLUS_mA]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_ARRAY_Y_MINUS_mA]);

	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_BUS_3v3_mA]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_BUS_5v_mA]);
	frame_put_u16(oframe, eps_adc_raw[EPS_ADC_BUS_12v_mA]);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *ss, frame_t * iframe, frame_t * oframe) {
	ss_power_state_t *ss_state = (ss_power_state_t*)ss->state;

	frame_put_bits_by_4(oframe, ss_state->low_voltage_counter, 4);

	frame_put_bits_by_4(oframe, (uint16_t)((eps_ADC_convertion(EPS_ADC_BATTERY_v,  eps_adc_raw[EPS_ADC_BATTERY_v]) - 6.2) * 100), 8);

	frame_put_bits_by_4(oframe, ((uint16_t)eps_ADC_convertion(EPS_ADC_BUS_3v3_mA, eps_adc_raw[EPS_ADC_BUS_3v3_mA])) >> 2, 8);
	frame_put_bits_by_4(oframe, ((uint16_t)eps_ADC_convertion(EPS_ADC_BUS_5v_mA,  eps_adc_raw[EPS_ADC_BUS_5v_mA])) >> 2, 8);

	frame_put_bits_by_4(oframe, eps_adc_raw[EPS_ADC_ARRAY_X_v] >> 6, 4);
	frame_put_bits_by_4(oframe, eps_adc_raw[EPS_ADC_ARRAY_Y_v] >> 6, 4);
	frame_put_bits_by_4(oframe, eps_adc_raw[EPS_ADC_ARRAY_Z_v] >> 6, 4);

	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	char buf[10];
	float voltage;

	frame_put_data(oframe, "Bat:", 4);
	if (RV_SUCCESS == POWER_get_battery_voltage(&voltage)) {
		int n;

		n = snprintf(buf, sizeof(buf), "%1.2fv ", voltage);
		if (n < 0) {
			return RV_ERROR;
		}

		return frame_put_data(oframe, buf, n);
	} else {
		return frame_put_data(oframe, "Err ", 4);
	}
}

static retval_t cmd_hard_reset_system(const  subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return POWER_reset();
}

static retval_t cmd_eps_get_all_adc_raw(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	int i;
	uint16_t version;
	uint16_t status;

	eps_GetFirmwareVersion(&version);
	frame_put_u16(oframe, version);

	update_raw_telemetry(true);

	for (i=0;i<ARRAY_COUNT(eps_adc_raw);i++) {
		frame_put_u16(oframe, eps_adc_raw[i]);
	}

	eps_GetStatus(&status);
	return frame_put_u16(oframe, status);
}

static retval_t cmd_turn_on_switch(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	retval_t rv;
	uint8_t switch_id;

	rv = frame_get_u8(iframe, &switch_id);
	if (RV_SUCCESS != rv) return rv;

	return POWER_turnOn(switch_id);
}

static retval_t cmd_turn_off_switch(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	retval_t rv;
	uint8_t switch_id;

	rv = frame_get_u8(iframe, &switch_id);
	if (RV_SUCCESS != rv) return rv;

	return POWER_turnOff(switch_id);
}

static retval_t cmd_set_matrix_key(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return frame_get_u32(iframe, &nvram.power.matrix_disable_key);
}

static retval_t cmd_eps_reset(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return eps_Reset();
}

static retval_t cmd_set_all_shifted(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint16_t switches;
	retval_t rv;

	rv = frame_get_u16(iframe, &switches);
	if (RV_SUCCESS != rv) return rv;

	frame_put_u16(oframe, 0);
	return POWER_set_all_shifted_switches(switches);
}

static retval_t cmd_ina_get_telemetry(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_put_u16(oframe, pd_3v3_status);
	frame_put_s16(oframe, (int16_t)pd_3v3_mV);
	frame_put_s16(oframe, (int16_t)pd_3v3_mA);
	frame_put_u16(oframe, pd_5v_status);
	frame_put_s16(oframe, (int16_t)pd_5v_mV);
	frame_put_s16(oframe, (int16_t)pd_5v_mA);
	frame_put_u16(oframe, pd_12v_status);
	frame_put_s16(oframe, (int16_t)pd_12v_mV);
	return frame_put_s16(oframe, (int16_t)pd_12v_mA);
}

static retval_t cmd_ina_reset(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	PD_ina_initialize(ch_ina_pd3v3, 3.3f, 1000.f);
	PD_ina_initialize(ch_ina_pd5v,  5.f,  1000.f);
	return PD_ina_initialize(ch_ina_pd12v, 12.f, 25000.f);
}

static retval_t cmd_eps_fast_sample(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	eps_adc_channel_t channel_id=0;
	uint16_t count;
	uint16_t value;
	retval_t rv;

	rv = frame_get_u8(iframe, (uint8_t *)&channel_id);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u16(iframe, &count);
	if (RV_SUCCESS != rv) return rv;

	while (count) {
		rv = eps_GetADCRaw(channel_id, &value);
		if (RV_SUCCESS != rv) value = 0xffff;

		rv = frame_put_u16(oframe, value);
		if (RV_SUCCESS != rv) return rv;
		count--;
	}
	return RV_SUCCESS;
}

const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("low_voltage_counter:u16, raw_battery_v:u16:[10.11521 - (raw_battery_v * 0.01185) min: 8.5 max: 6], nice_battery_v:u16:[(nice_battery_v /1000.0) min: 8.5 max: 6], battery_mA:u16:[8380.809 - (battery_mA * 8.20101)], array_1_v:u16:[35.01082 - (array_1_v * 0.03606)], array_1p_mA:u16:[691.3017 - (array_1p_mA * 0.54012)], array_1m_mA:u16:[692.3604 - (array_1m_mA * 0.53886)], array_2_v:u16:[34.5597 - (array_2_v * 0.03538)], array_2p_mA:u16:[694.3171 - (array_2p_mA * 0.54414)], array_2m_mA:u16:[695.4475 - (array_2m_mA * 0.54869)], _3v3_mA:u16:[4623.646 - (_3v3_mA * 5.43512)], _5v_mA:u16:[5262.442 - (_5v_mA * 5.39879)], _12v_mA:u16:[5284.683 - (_12v_mA * 5.40332)]", ""),
	DECLARE_COMMAND(SS_CMD_POWER_HARD_RESET_SYSTEM, cmd_hard_reset_system, "reset", "Power-cycle the whole system", "", ""),
	DECLARE_COMMAND(SS_CMD_POWER_EPS_GET_ALL_ADC_RAW, cmd_eps_get_all_adc_raw, "epsGetADC", "Get EPS version, and ADC last reading", "", "version:u16,adc0:u16,adc1:u16,adc2:u16,adc3:u16,adc4:u16,adc5:u16,adc6:u16,adc7:u16,adc8:u16,adc9:u16,adc10:u16,adc11:u16,adc12:u16,adc13:u16,adc14:u16,adc15:u16,adc16:u16,adc17:u16,adc18:u16,adc19:u16,adc20:u16,adc21:u16,adc22:u16,adc23:u16,adc24:u16,adc25:u16,adc26:u16,adc27:u16,adc28:u16,adc29:u16,adc30:u16,adc31:u16,status:u16"),
	DECLARE_COMMAND(SS_CMD_POWER_TURN_ON_SWITCH, cmd_turn_on_switch, "turnOn", "Turn on one switch, <switch number>", "switch:u8", ""),
	DECLARE_COMMAND(SS_CMD_POWER_TURN_OFF_SWITCH, cmd_turn_off_switch, "turnOff", "Turn off one switch <switch number>", "switch:u8", ""),
	DECLARE_COMMAND(SS_CMD_POWER_SET_MATRIX_KEY, cmd_set_matrix_key, "setMatrix", "Set Power matrix disable key <secret key>", "key:u32", ""),
	DECLARE_COMMAND(SS_CMD_EPS_RESET, cmd_eps_reset, "epsReset", "Resets the EPS microprocessor", "", ""),
	DECLARE_COMMAND(SS_CMD_POWER_SET_ALL_SWITCHES, cmd_set_all_shifted, "set", "Set all switches switches", "shiftedSwitches:u16","oldSwitches:u16"),
	DECLARE_COMMAND(SS_CMD_INA_GET_TELEMETRY, cmd_ina_get_telemetry, "inaTelemetry", "Get all telemetry from the three INAs", "", "PD_3v3_status:u16,PD_3v3_mV:s16,PD_3v3_mA:s16,PD_5v_status:u16,PD_5v_mV:s16,PD_5v_mA:s16,PD_12v_status:u16,PD_12v_mV:s16,PD_12v_mA:s16"),
	DECLARE_COMMAND(SS_CMD_INA_RESET, cmd_ina_reset, "inaReset", "reset and reconfigure all three INAs", "", ""),
	DECLARE_COMMAND(SS_CMD_EPS_FAST_SAMPLE, cmd_eps_fast_sample, "epsFastSample", "Burst sample an EPS channel", "channel:u8,count:u16", "values:u16[]"),
};

static subsystem_api_t subsystem_api = {
    .main_task = &POWER_main_task,
    .command_execute = &ss_command_execute,
};

extern const ss_tests_t power_tests;

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_POWER,
    .usStackDepth = STACK_DEPTH_POWER,
    .id = SS_POWER,
    .name = "POWER",
    .tests = &power_tests,
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static ss_power_state_t subsystem_state;

subsystem_t SUBSYSTEM_POWER = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = (subsystem_state_t*)&subsystem_state
};
