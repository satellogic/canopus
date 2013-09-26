#ifndef _CANOPUS_SUBSYSTEM_POWER_H_
#define _CANOPUS_SUBSYSTEM_POWER_H_

#include <canopus/subsystem/subsystem.h>

typedef enum power_switch_e {
	// Torino 1500
	POWER_SW_OVERO				= 0,
	POWER_SW_TMS_OTHER			= 1,

	// Octopus / Service a Board
	POWER_SW_IMU				= 2,
	POWER_SW_HEATER_1			= 3,
	POWER_SW_HEATER_2			= 4,
	POWER_SW_HEATER_3			= 5,
	POWER_SW_HEATER_4			= 6,
	POWER_SW_HEATER_5			= 7,
	POWER_SW_HEATER_6			= 8,
	POWER_SW_HEATER_7			= 9,
	POWER_SW_HEATER_8			= 10,
	POWER_SW_ANTENNA_DEPLOY_1	= 11,
	POWER_SW_ANTENNA_DEPLOY_2	= 12,
	POWER_SW_ANTENNA_DEPLOY_3	= 13,
	POWER_SW_ANTENNA_DEPLOY_4	= 14,
	POWER_SW_PD_TESTER_A		= 15,
	POWER_SW_PD_TESTER_B		= 16,
	POWER_SW_PD_TESTER_C		= 17,
	POWER_SW_PD_TESTER_D		= 18,

	POWER_SW_PD_3v3				= 19,
	POWER_SW_PD_5v				= 20,
	POWER_SW_PD_12v				= 21,

	// Chirimbolo / Payload board
	POWER_SW_SVIP				= 22,
	POWER_SW_CONDOR				= 23,
	POWER_SW_GPS				= 24,
	POWER_SW_SDR				= 25,

    POWER_SW_COUNT
} power_switch_e;

#define POWER_SWITCH_BIT(_switch_id) (1 << (_switch_id - 1))

extern subsystem_t SUBSYSTEM_POWER;

typedef struct nvram_power_t {
	uint32_t telemetry_update_time_s;
	float  battery_low_voltage_v;
	float  battery_ok_voltage_v;
	float  battery_mission_voltage_v;
	uint32_t matrix_disable_key;
	bool imu_power_forced_off;
} nvram_power_t;

typedef struct ss_power_state_t {
    subsystem_state_t ss_state;
    uint32_t wanted_switches_state;
    uint16_t current_trip_counter[POWER_SW_COUNT];
    uint16_t low_voltage_counter;
} ss_power_state_t;

retval_t POWER_turnOn(power_switch_e switch_id);
retval_t POWER_turnOff(power_switch_e switch_id);
retval_t POWER_watchdog_restart_counter();
retval_t POWER_reset(void);
retval_t POWER_get_battery_voltage(float *voltage);
#endif /* _CANOPUS_SUBSYSTEM_POWER_H_ */
