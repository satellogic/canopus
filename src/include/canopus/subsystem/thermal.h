#ifndef __THERMAL_H__
#define __THERMAL_H__

extern subsystem_t SUBSYSTEM_THERMAL;

#define UNREGULATED_ZONE		-200000

typedef enum thermal_sensors_e {
	/* BANK 0 at ADC channels */
	THERMAL_SENSOR_TMS0_REG          = 0,
	THERMAL_SENSOR_TMS0              = 1,
	THERMAL_SENSOR_OVERO             = 2,
	THERMAL_SENSOR_STRUCTURE         = 3,  /* H1.39 */
	THERMAL_SENSOR_SOLAR_Ym_OUTER    = 4,  /* H1.37 */
	THERMAL_SENSOR_CAMERA_BOARD      = 5,  /* H1.35 */
	THERMAL_SENSOR_BUS_PAYLOAD_BOARD = 6,  /* H1.33 */
	THERMAL_SENSOR_CAMERA_HOUSING    = 7,  /* H1.31 */
	THERMAL_SENSOR_SVIP              = 8,  /* H1.29 */
	THERMAL_SENSOR_SOLAR_Xp_INNER    = 9,  /* H1.27 */
	THERMAL_SENSOR_SOLAR_Xp_OUTER    = 10,  /* H1.25 */
	THERMAL_SENSOR_RADIO             = 11,  /* H1.23 */

	THERMAL_SENSOR_BATTERY,				/* Embedded in EPS */
	THERMAL_SENSOR_RADIO_CPU,			/* Embedded in Li-1 */
	THERMAL_SENSOR_IMU,					/* Embedded in ADIS-16405 */
} thermal_sensors_e;

typedef enum thermal_zones_t {
	THERMAL_ZONE_STARTRACKER,
	THERMAL_ZONE_LOWER_1,
	THERMAL_ZONE_LOWER_2,

	THERMAL_ZONE_COUNT,
} thermal_zones_t;

typedef struct thermal_limits_t {
	int32_t turn_on_heaters_temperature;
	int32_t turn_off_heaters_temperature;
} thermal_limits_t;

typedef struct nvram_thermal_t {
	thermal_limits_t thermal_limits_survival[THERMAL_ZONE_COUNT];
	thermal_limits_t thermal_limits_mission[THERMAL_ZONE_COUNT];
	thermal_limits_t thermal_limits_low_power[THERMAL_ZONE_COUNT];
	uint32_t matrix_disable_key;
} nvram_thermal_t;

#define DEFAULT_THERMAL_LIMITS_SURVIVAL {		/* All values in °C * 1000 (mC) */	\
	[THERMAL_ZONE_STARTRACKER] = {								\
			.turn_on_heaters_temperature  = UNREGULATED_ZONE,	\
			.turn_off_heaters_temperature = UNREGULATED_ZONE},	\
	[THERMAL_ZONE_LOWER_1] = {									\
			.turn_on_heaters_temperature  = -30000,				\
			.turn_off_heaters_temperature = -29000},			\
	[THERMAL_ZONE_LOWER_2] = {									\
			.turn_on_heaters_temperature  = -30000,				\
			.turn_off_heaters_temperature = -29000},			\
	}

#define DEFAULT_THERMAL_LIMITS_MISSION {						\
	[THERMAL_ZONE_STARTRACKER] = {								\
			.turn_on_heaters_temperature  = UNREGULATED_ZONE,	\
			.turn_off_heaters_temperature = UNREGULATED_ZONE},	\
	[THERMAL_ZONE_LOWER_1] = {									\
			.turn_on_heaters_temperature  = -30000,				\
			.turn_off_heaters_temperature = -29000},			\
	[THERMAL_ZONE_LOWER_2] = {									\
			.turn_on_heaters_temperature  = -30000,				\
			.turn_off_heaters_temperature = -29000},			\
	}

#define DEFAULT_THERMAL_LIMITS_LOW_POWER {						\
	[THERMAL_ZONE_STARTRACKER] = {								\
			.turn_on_heaters_temperature  = UNREGULATED_ZONE,	\
			.turn_off_heaters_temperature = UNREGULATED_ZONE},	\
	[THERMAL_ZONE_LOWER_1] = {									\
			.turn_on_heaters_temperature  = UNREGULATED_ZONE,	\
			.turn_off_heaters_temperature = UNREGULATED_ZONE},	\
	[THERMAL_ZONE_LOWER_2] = {									\
			.turn_on_heaters_temperature  = UNREGULATED_ZONE,	\
			.turn_off_heaters_temperature = UNREGULATED_ZONE},	\
	}

retval_t THERMAL_get_temperature(thermal_sensors_e sensor, int32_t *temperature_milli_celcious);

#endif /* __THERMAL_H__ */
