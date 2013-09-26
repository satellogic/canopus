#ifndef _CANOPUS_SUBSYSTEM_ADCS_H_
#define _CANOPUS_SUBSYSTEM_ADCS_H_

#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/subsystem/aocs/detumbling.h>
#include <canopus/subsystem/aocs/css.h>
#include <canopus/drivers/imu/adis16xxx.h>

#define BDOT_GYROSCOPE_DEADZONE_LIMIT	(0.25f)

/* 75 dps, 64 taps */
#define IMU_DEFAULT_SENS_AVG 0x0106
#define IMU_DEFAULT_XGYRO_OFF 0
#define IMU_DEFAULT_YGYRO_OFF 0
#define IMU_DEFAULT_ZGYRO_OFF 0
#define IMU_DEFAULT_XACCL_OFF 0
#define IMU_DEFAULT_YACCL_OFF 0
#define IMU_DEFAULT_ZACCL_OFF 0
#define IMU_DEFAULT_XMAGN_HIF_OFF 199
#define IMU_DEFAULT_YMAGN_HIF_OFF 0x3fb9 // -71
#define IMU_DEFAULT_ZMAGN_HIF_OFF 119
#define IMU_DEFAULT_XMAGN_SIF_OFF 0x800
#define IMU_DEFAULT_YMAGN_SIF_OFF 0x800
#define IMU_DEFAULT_ZMAGN_SIF_OFF 0x800

#define IMU_DEFAULT_DATA_RDY_BITS 0x0006

typedef enum detumbling_t {
	AOCS_DETUMBLING_BXW,
} detumbling_t;

typedef enum run_sampler_t {
	AOCS_START_CDH_SAMPLER = 0x69,
	AOCS_DONTTT_START_CDH_SAMPLER = 0x42,
} run_sampler_t;

typedef enum aocs_mode_t {
	AOCS_POINTING = 4,
	AOCS_LOW_POWER = 3,
	AOCS_IMU_FAILURE = 2,
	AOCS_SPI_FAILURE = 1,
	AOCS_DETUMBLING = 0,
	AOCS_MTQ_OFF = 0xdead40c5,
} aocs_mode_t;

#define BROKEN_IMU(__mode) ((__mode == AOCS_SPI_FAILURE) || (__mode == AOCS_IMU_FAILURE))
#define AOCS_IS_OFF(__mode) ((__mode == AOCS_MTQ_OFF) || (__mode == AOCS_LOW_POWER))

/* This are CANOPUS modes not AOCS modes */
#define SATELLITE_MODE_WITH_AOCS(__mode) ((__mode == SM_MISSION) || (__mode == SM_SURVIVAL) || (__mode == SM_LOW_POWER))

extern detumbling_t AOCS_DEFAULT_DETUMBLING;

/* Number of steps to wait before trying to turn detumbling on
 * after failure.
 */
#define AOCS_NO_DETUMBLING_RETRY 20

extern subsystem_t SUBSYSTEM_AOCS;
extern volatile uint32_t imu_dready_count;

#define IMU_DREADY_COUNT_SAMPLE (10)
#define IMU_STEPS_MTQOFF_BEFORE (2)

bool imu_dready_event(void);

typedef struct sampler_conf_t {
	uint16_t offset;
	uint16_t size;
	uint16_t count;
	uint16_t interval;
} sampler_conf_t;

typedef struct mtq_axis_map_t {
	uint8_t x_channel;
	int8_t x_sign;
	uint8_t y_channel;
	int8_t y_sign;
	uint8_t z_channel;
	int8_t z_sign;
} mtq_axis_map_t;

typedef struct lovera_prefeed_conf_t {
	float epsilon;
	float beta;
	float kp;
	float kv;
} lovera_prefeed_conf_t;

typedef struct nvram_aocs_t {
    float Bxw_gain;
    detumbling_t detumbling;
    uint8_t no_mtq_actuate;
    uint32_t default_controller_frequency_ms;
    run_sampler_t run_sampler;
    aocs_mode_t default_mode;
    matrixf_t magcal_matrix_signed_raw;
    float max_survival_dipole;
    sampler_conf_t sampler_configuration;
    mtq_axis_map_t mtq_axis_map;
    lovera_prefeed_conf_t lovera_prefeed_conf;
} nvram_aocs_t;

typedef struct aocs_state_t {
	adis1640x_data_burst_raw last_imu_raw;
	sun_data_t sun_data;
	bool imu_measurement_without_mtq;
	uint8_t steps_for_recovery;
    uint32_t current_controller_frequency_ms;
	bool mtq_on;
    aocs_mode_t mode;
    bool imu_task_running;
    bool measuring_semaphore_created;
    double adc_fix_scale;
    bool breakage;
} aocs_state_t;

#define VEC2NUM_x (0)
#define VEC2NUM_y (1)
#define VEC2NUM_z (2)

#define LASTB_RAW(__axis) (\
	nvram.aocs.magcal_matrix_signed_raw[VEC2NUM_##__axis][VEC2NUM_x]*RAW2LSB14(aocs_state.last_imu_raw.mag_x)+\
	nvram.aocs.magcal_matrix_signed_raw[VEC2NUM_##__axis][VEC2NUM_y]*RAW2LSB14(aocs_state.last_imu_raw.mag_y)+\
	nvram.aocs.magcal_matrix_signed_raw[VEC2NUM_##__axis][VEC2NUM_z]*RAW2LSB14(aocs_state.last_imu_raw.mag_z)\
)

#define LASTW_RAW(__axis) (RAW2LSB14(aocs_state.last_imu_raw.gyro_##__axis))
#define LASTT_RAW (RAW2LSB12(aocs_state.last_imu_raw.temp))

#define LASTB(__axis) (LASTB_RAW(__axis) * ADIS16400_MAGN_SCALE)
#define LASTW(__axis) (LASTW_RAW(__axis) * ADIS16400_GYRO_SCALE_075)

// Pointing
#define AOCS_INERTIA MATRIXF_IDENTITY

// TODO: Move this to a better place
#define GPIO_DREADY_PORT		gioPORTA
#define GPIO_DREADY_BIT		2

retval_t AOCS_get_imu_temperature(int32_t *temperature_mC);
retval_t AOCS_get_gyro_magnitude(float *gyroMag_ds);
#endif
