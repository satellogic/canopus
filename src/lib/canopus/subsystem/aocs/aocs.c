#include <canopus/types.h>
#include <canopus/logging.h>
#include <canopus/floatfmt.h>
#include <canopus/drivers/magnetorquer.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/imu/adis16xxx.h>
#include <canopus/drivers/tms570/pwm.h>
#include <canopus/board/channels.h>
#include <canopus/board/adc.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/platform.h>
#include <canopus/subsystem/cdh.h> // for sampler
#include <canopus/subsystem/aocs/aocs.h>
#include <canopus/subsystem/aocs/detumbling.h>
#include <canopus/subsystem/aocs/pointing.h>
#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/nvram.h>

#include <math.h>

#include <string.h>

static aocs_state_t aocs_state;

adis1640x_data_burst_raw imu_raw_error = {0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead, 0xdead};

/* Statics */
static void to_mode_imu_failure(retval_t rv);
static void to_mode_spi_failure(retval_t rv);
static void aocs_mtq_off();
static void aocs_mtq_on();

static retval_t configure_imu();
static retval_t check_imu_configuration();
static retval_t reset_imu(subsystem_t *ss);

/* Locks */
static bool mtq_not_initialized = true;
static bool adc_not_initialized = true;
xSemaphoreHandle xSemaphore_imu_dready;
xSemaphoreHandle xSemaphore_measuring;

detumbling_t AOCS_DEFAULT_DETUMBLING = AOCS_DETUMBLING_BXW;

static retval_t imu_dready_task_rv = RV_ERROR;
volatile uint32_t imu_dready_count = 0;

/* Open and close IMU SPI channel when turning it on/off */
//#define AOCS_IMU_CHANNEL_OPENCLOSE
/* Do only one channel open when initializing, then never close it */
#undef AOCS_IMU_CHANNEL_OPENCLOSE

#ifdef AOCS_IMU_CHANNEL_OPENCLOSE
#define AOCS_OPEN_CHANNEL_ON_LOW_POWER_AND_RESET (true)
#else
#define AOCS_OPEN_CHANNEL_ON_LOW_POWER_AND_RESET (false)
#endif

#define AOCS_CIRCULAR_BUFFER
//#undef AOCS_CIRCULAR_BUFFER

#define AOCS_WITH_SAMPLER
//#undef AOCS_WITH_SAMPLER

#ifdef AOCS_CIRCULAR_BUFFER
#define AOCS_CIRCULAR_BUFFER_SIZE_IN_BURSTS ((32*1024) / sizeof(adis1640x_data_burst_raw))
static uint32_t aocs_circular_buffer_size = AOCS_CIRCULAR_BUFFER_SIZE_IN_BURSTS;
static adis1640x_data_burst_raw aocs_circular_buffer[AOCS_CIRCULAR_BUFFER_SIZE_IN_BURSTS];

static bool log_to_aocs_buffer = false;
static uint32_t aocs_circular_buffer_pointer = 0;
#endif

static void css_read_vector() {
	float samples[ADC_CHANNELS];

	adc_get_samples(ADC_BANK_SOLAR, samples);

	aocs_state.sun_data.css_adc_measurement_volts.x_neg =
			samples[ADC_CHANNEL_SOLAR_X_NEG];
	aocs_state.sun_data.css_adc_measurement_volts.y_neg =
			samples[ADC_CHANNEL_SOLAR_Y_NEG];
	aocs_state.sun_data.css_adc_measurement_volts.z_neg =
			samples[ADC_CHANNEL_SOLAR_Z_NEG];
	aocs_state.sun_data.css_adc_measurement_volts.x_pos =
			samples[ADC_CHANNEL_SOLAR_X_POS];
	aocs_state.sun_data.css_adc_measurement_volts.y_pos =
			samples[ADC_CHANNEL_SOLAR_Y_POS];
	aocs_state.sun_data.css_adc_measurement_volts.z_pos =
			samples[ADC_CHANNEL_SOLAR_Z_POS];

	fill_up_sun_t(&(aocs_state.sun_data),
			&(aocs_state.sun_data.css_adc_measurement_volts));
}

bool imu_dready_event(void){
	imu_dready_count++;
	return (((imu_dready_count + IMU_STEPS_MTQOFF_BEFORE) % IMU_DREADY_COUNT_SAMPLE == 0) || (imu_dready_count % IMU_DREADY_COUNT_SAMPLE == 0));
}

static void imu_dready_task(void *ss) {
#define IMU_SAMPLES_TO_CSS_SAMPLE 1
	uint16_t imu_samples_count = 0;

	aocs_state.imu_task_running = true;

	while (1) {
		PLATFORM_ss_is_alive(ss, HEARTBEAT_2nd_TASK);
		xSemaphoreTake(xSemaphore_imu_dready, 10000 / portTICK_RATE_MS);
		/* 10s timeout to keep watchdog updated         ^  */
		if BROKEN_IMU(aocs_state.mode) continue;
		/* This ^ happens in POSIX but should never happen in hardware */
		if AOCS_IS_OFF(aocs_state.mode) continue;

		/* This task won't work without the measuring semaphore */
		if(!aocs_state.measuring_semaphore_created) {
			xSemaphore_measuring = xSemaphoreCreateMutex();
			if(xSemaphore_measuring != NULL) {
				aocs_state.measuring_semaphore_created = true;
			} else {
				continue;
			}
		}

		if((imu_dready_count + IMU_STEPS_MTQOFF_BEFORE) % IMU_DREADY_COUNT_SAMPLE == 0) {
			xSemaphoreTake(xSemaphore_measuring, 1000 / portTICK_RATE_MS);
			aocs_mtq_off();
		}

		if(imu_dready_count % IMU_DREADY_COUNT_SAMPLE == 0) {

			imu_dready_task_rv = adis1640x_read_burst(ch_adis, &(aocs_state.last_imu_raw));
			if (RV_SUCCESS != imu_dready_task_rv) to_mode_imu_failure(imu_dready_task_rv);

			imu_samples_count++;

#ifdef AOCS_CIRCULAR_BUFFER
			if(log_to_aocs_buffer) {
				aocs_circular_buffer[aocs_circular_buffer_pointer++] =
						aocs_state.last_imu_raw;
				if(aocs_circular_buffer_pointer == aocs_circular_buffer_size)
					aocs_circular_buffer_pointer = 0;
			}
#endif
			xSemaphoreGive(xSemaphore_measuring);
			aocs_mtq_on();
		}

		if(imu_samples_count == IMU_SAMPLES_TO_CSS_SAMPLE) {
			css_read_vector();
			imu_samples_count = 0;
		}
	}
}

static retval_t initialize_imu(subsystem_t *ss, bool imu_channel_open) {
	retval_t rv = RV_ERROR;

	/* ADIS 16405 documentation Rev. B page 4 */
	vTaskDelay(220 / portTICK_RATE_MS);

	if(imu_channel_open) {
		rv = channel_open(ch_adis);
		if (RV_SUCCESS != rv) return rv;
	}

	// ToDo: 1024 must be changed to the propper stack size, 4 must be changed to the propper priority
	if(!aocs_state.imu_task_running)
		if (pdPASS != xTaskCreate(imu_dready_task, (signed char *)"IMU DREADY handler", 1024, (void*)ss, 4, NULL));

	return RV_SUCCESS;
}

static retval_t aocs_hardware_on(subsystem_t *ss, bool imu_channel_open) {
    /* temp kludge (phil) */
    extern retval_t board_magnetometer_init();
    extern retval_t board_gyroscope_init();
    retval_t rv = RV_SUCCESS;
    bool check_imu = true;

	log_report(LOG_SS_AOCS_VERBOSE, "AOCS turning on hardware\n");

	rv = initialize_imu(ss, imu_channel_open);
	if (RV_SUCCESS != rv) {
		to_mode_spi_failure(rv);
		check_imu = false;
	}

	if(check_imu) {
		rv = check_imu_configuration();
		if (RV_SUCCESS != rv) rv = configure_imu();
		rv = check_imu_configuration();
		if (RV_SUCCESS != rv) {
			// This rv is returned. Beware when checking TODO below.
			to_mode_imu_failure(rv);
		}
	}

    if(mtq_not_initialized) {
    	pwm_init(); // TODO(peter) rv? call here? call on board?
    	mtq_not_initialized = false;
    }

    if(adc_not_initialized) {
    	adc_init(); // TODO(peter) rv? call here? call on board?
    	adc_not_initialized = false;
    }

    log_report(LOG_SS_AOCS_VERBOSE, "AOCS FINISHED turning on hardware\n");

    return rv;
}

static retval_t aocs_hardware_off(subsystem_t *ss) {
    /* temp kludge (phil) */
    extern retval_t board_magnetometer_init();
    extern retval_t board_gyroscope_init();
    retval_t rv = RV_SUCCESS;

	log_report(LOG_SS_AOCS_VERBOSE, "AOCS turning off hardware\n");

#ifdef AOCS_IMU_CHANNEL_OPENCLOSE
	rv = channel_close(ch_adis);
	if (RV_SUCCESS != rv) return rv;
#endif

    if(!mtq_not_initialized) {
    	aocs_mtq_off();
    }

    log_report(LOG_SS_AOCS_VERBOSE, "AOCS FINISHED turning off hardware\n");

    return rv;
}

static void to_mode_imu_failure(retval_t rv)
{
	aocs_state.mode = AOCS_IMU_FAILURE;
	aocs_state.current_controller_frequency_ms = 10000; /* ms */
}

static void to_mode_spi_failure(retval_t rv)
{
	aocs_state.mode = AOCS_SPI_FAILURE;
	aocs_state.current_controller_frequency_ms = 10000; /* ms */
}

static void to_mode_low_power(subsystem_t *ss) {
	aocs_hardware_off(ss);
	POWER_turnOff(POWER_SW_IMU);

	aocs_state.mode = AOCS_LOW_POWER;
	aocs_state.current_controller_frequency_ms = 10000; /* ms */
}

static void to_mode_detumbling(subsystem_t *ss) {
	aocs_state.mode = AOCS_DETUMBLING;
	aocs_state.current_controller_frequency_ms = nvram.aocs.default_controller_frequency_ms;
}

static void to_mode_pointing(subsystem_t *ss) {
	aocs_state.mode = AOCS_POINTING;
	aocs_state.current_controller_frequency_ms = nvram.aocs.default_controller_frequency_ms;
}

static retval_t configure_imu()
{
	retval_t rv = RV_ERROR;

	rv = adis1640x_write_register(ch_adis, ADIS16400_SENS_AVG, IMU_DEFAULT_SENS_AVG);
	if (rv != RV_SUCCESS) return rv;
	rv = adis1640x_write_register(ch_adis, ADIS16400_MSC_CTRL, IMU_DEFAULT_DATA_RDY_BITS);
	if (rv != RV_SUCCESS) return rv;

	rv = adis1640x_write_register(ch_adis, ADIS16400_XMAGN_HIF_OFF, IMU_DEFAULT_XMAGN_HIF_OFF);
	if (rv != RV_SUCCESS) return rv;
	rv = adis1640x_write_register(ch_adis, ADIS16400_YMAGN_HIF_OFF, IMU_DEFAULT_YMAGN_HIF_OFF);
	if (rv != RV_SUCCESS) return rv;
	rv = adis1640x_write_register(ch_adis, ADIS16400_ZMAGN_HIF_OFF, IMU_DEFAULT_ZMAGN_HIF_OFF);
	if (rv != RV_SUCCESS) return rv;

	return RV_SUCCESS;
}

#define CHECK_OFFSET(__rv, __axis, __sensor, __value) do {\
__rv = adis1640x_read_register(ch_adis, ADIS16400_##__axis##__sensor##_OFF, &__value);\
if (__rv != RV_SUCCESS) { break; }\
if((__value & ADIS16400_##__sensor##_OFF_MASK) != (ADIS16400_##__sensor##_OFF_MASK & IMU_DEFAULT_##__axis##__sensor##_OFF))\
__rv = RV_ERROR;} while(false);

static retval_t check_imu_configuration()
{
	retval_t rv = RV_ERROR;
	uint16_t value;

	/* Check SENS_AVG */
	rv = adis1640x_read_register(ch_adis, ADIS16400_SENS_AVG, &value);
	if (rv != RV_SUCCESS) return rv;

	if((ADIS16400_SENS_AVG_RANGE & value) != (ADIS16400_SENS_AVG_RANGE & IMU_DEFAULT_SENS_AVG))
		return RV_ERROR;

	if((ADIS16400_SENS_AVG_TAPS & value) != (ADIS16400_SENS_AVG_TAPS & IMU_DEFAULT_SENS_AVG))
		return RV_ERROR;
	/* End of check SENS_AVG */

	/* Check calibration biases */
	CHECK_OFFSET(rv, X, GYRO, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Y, GYRO, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Z, GYRO, value)
	if (rv != RV_SUCCESS) return rv;

	CHECK_OFFSET(rv, X, ACCL, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Y, ACCL, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Z, ACCL, value)
	if (rv != RV_SUCCESS) return rv;

	CHECK_OFFSET(rv, X, MAGN_HIF, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Y, MAGN_HIF, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Z, MAGN_HIF, value)
	if (rv != RV_SUCCESS) return rv;

	CHECK_OFFSET(rv, X, MAGN_SIF, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Y, MAGN_SIF, value)
	if (rv != RV_SUCCESS) return rv;
	CHECK_OFFSET(rv, Z, MAGN_SIF, value)
	if (rv != RV_SUCCESS) return rv;
	/* End check calibration biases */

	/* Check MSC_CTL (data ready)*/
	rv = adis1640x_read_register(ch_adis, ADIS16400_MSC_CTRL, &value);
	if (rv != RV_SUCCESS) return rv;

	if((value & ADIS16400_MSC_CTRL_DATA_RDY_BITS) != (ADIS16400_MSC_CTRL_DATA_RDY_BITS & IMU_DEFAULT_DATA_RDY_BITS))
		return RV_ERROR;
	/* End of check MSC_CTL */


	return RV_SUCCESS;
}

#ifdef AOCS_WITH_SAMPLER
static void aocs_run_sampler(subsystem_t *ss) {
	retval_t rv;

    /* Start CDH AOCS beacon sampling */
   	log_report(LOG_SS_AOCS_VERBOSE, "Initiating CDH sample\n");

   	uint16_t sample_params[] = {
						0xdead,
						0x2ea2,
						0x3ea3,
						0x4ea4,
	};
	frame_t cdh_sample_params = DECLARE_FRAME_SIZE(&sample_params,
			sizeof(sample_params));
	frame_put_u16(&cdh_sample_params, nvram.aocs.sampler_configuration.offset);
	frame_put_u16(&cdh_sample_params, nvram.aocs.sampler_configuration.size);
	frame_put_u16(&cdh_sample_params, nvram.aocs.sampler_configuration.count);
	frame_put_u16(&cdh_sample_params, nvram.aocs.sampler_configuration.interval);  /* interval (1 min)   */
	frame_reset_for_reading(&cdh_sample_params);
	rv = cmd_sample_beacon(NULL, &cdh_sample_params, NULL);
	log_report_fmt(LOG_SS_AOCS_VERBOSE, "Initiated CDH sample: %s\n", retval_s(rv));

	uint8_t flash_params[] = {0x42};
	frame_t cdh_flash_when_done_params = DECLARE_FRAME_SIZE(&flash_params,
			sizeof(flash_params));
	frame_put_u8(&cdh_flash_when_done_params, 1); /* True */
	frame_reset_for_reading(&cdh_flash_when_done_params);
	rv |= cmd_sample_flash_when_done(NULL, &cdh_flash_when_done_params, NULL);
	log_report_fmt(LOG_SS_AOCS_VERBOSE, "CDH sample to flash: %s\n", retval_s(rv));

	if(!rv) nvram.aocs.run_sampler = AOCS_DONTTT_START_CDH_SAMPLER;
	MEMORY_nvram_save(&nvram.aocs.run_sampler, sizeof(nvram.aocs.run_sampler));

}
#endif

static void aocs_mtq_off() {
	if(aocs_state.mtq_on) {
		pwm_alloff();
		aocs_state.mtq_on = false;
	}
}

static void aocs_mtq_on() {
	if(!aocs_state.mtq_on) {
		pwm_allon();
		aocs_state.mtq_on = true;
	}
}

static void aocs_mtq_actuate_duty(const vectorf_t control_dipole) {
    char fbuf[3][FLOATFMT_BUFSZ_MAX];

    ftoa(control_dipole[0], fbuf[0], sizeof(fbuf[0]));
    ftoa(control_dipole[1], fbuf[1], sizeof(fbuf[1]));
    ftoa(control_dipole[2], fbuf[2], sizeof(fbuf[2]));

	pwm_set_duty_vec(control_dipole);
	aocs_state.mtq_on = true;
    pwm_allon();
}

static void aocs_mtq_actuate_dipole(const vectorf_t control_dipole) {
    vectorf_t duty_cycle_dipole;

    if(nvram.aocs.no_mtq_actuate) return;

    /* Control_dipole is in IMU coordinates.
     * Convert to MTQ coordinates (chan 0, 1 ,2).
     */

    duty_cycle_dipole[0] = nvram.aocs.mtq_axis_map.x_sign * control_dipole[nvram.aocs.mtq_axis_map.x_channel];
    duty_cycle_dipole[1] = nvram.aocs.mtq_axis_map.y_sign * control_dipole[nvram.aocs.mtq_axis_map.y_channel];
    duty_cycle_dipole[2] = nvram.aocs.mtq_axis_map.z_sign * control_dipole[nvram.aocs.mtq_axis_map.z_channel];

    smult(100., duty_cycle_dipole);
    /* All same dipole: smult(1./MTQ_DIPOLE_MAX, duty_cycle_dipole); */
    duty_cycle_dipole[0] *=  1./MTQ_DIPOLE_MAX_0;
    duty_cycle_dipole[1] *=  1./MTQ_DIPOLE_MAX_1;
    duty_cycle_dipole[2] *=  1./MTQ_DIPOLE_MAX_2;

    aocs_mtq_actuate_duty(duty_cycle_dipole);
}

static void detumbling_step(subsystem_t *ss) {
    vectorf_t control_dipole = {0., 0., 0.};
    vectorf_t B,W;

    assert(aocs_state.mode == AOCS_DETUMBLING);
    //assert(aocs_state.imu_measurement_without_mtq);

    B[0] = LASTB(x); B[1] = LASTB(y); B[2] = LASTB(z);
    W[0] = LASTW(x); W[1] = LASTW(y); W[2] = LASTW(z);
  	Bxw_mtq_dipole(B, W, control_dipole);

    aocs_mtq_actuate_dipole(control_dipole);
}

static retval_t pointing_step(subsystem_t *ss) {
    vectorf_t control_dipole = {0., 0., 0.};
    vectorf_t Bb,Sb, Qv, W, Bi, Si;
    float Qs;
    matrixf_t A;
    retval_t rv;

    assert(aocs_state.mode == AOCS_POINTING);
    //assert(aocs_state.imu_measurement_without_mtq);

    Bb[0] = LASTB(x); Bb[1] = LASTB(y); Bb[2] = LASTB(z);
    Sb[0] = aocs_state.sun_data.sun_vector.x;
    Sb[1] = aocs_state.sun_data.sun_vector.y;
    Sb[2] = aocs_state.sun_data.sun_vector.z;

    rv = RV_ERROR; // return if future hook not set
    FUTURE_HOOK_3(get_sun_magnetic_inertial, &rv, &Si, &Bi);
    SUCCESS_OR_RETURN(rv);

    triad(Sb, Si, Bb, Bi, A);
    rotmat2quat(A, Qv, &Qs);

    FUTURE_HOOK_1(rotate_quaternion, &Qv);
    W[0] = LASTW(x); W[1] = LASTW(y); W[2] = LASTW(z);
    lovera_mtq_dipole(Qv, W, Bb, control_dipole);

    rv = RV_SUCCESS; // don't return if future hook not set
    FUTURE_HOOK_2(fix_dipole, &rv, &control_dipole);
    SUCCESS_OR_RETURN(rv);

    aocs_mtq_actuate_dipole(control_dipole);

    return RV_SUCCESS;
}

static retval_t reset_imu(subsystem_t *ss) {
	retval_t rv;

#ifdef AOCS_IMU_CHANNEL_OPENCLOSE
	rv = channel_close(ch_adis);
	if (RV_SUCCESS != rv) return rv;
#endif

	rv = initialize_imu(ss, AOCS_OPEN_CHANNEL_ON_LOW_POWER_AND_RESET);
	if (RV_SUCCESS != rv) return rv;

	return RV_SUCCESS;
}

static void AOCS_main_task(void *pvParameters) {
	satellite_mode_e mode = SM_BOOTING;
	satellite_mode_e prev_mode;
	retval_t rv = RV_ERROR; /* To call functions and store rv */
	bool imu_channel_openclose;

	subsystem_t * ss = (subsystem_t *)pvParameters;

	aocs_state.current_controller_frequency_ms = nvram.aocs.default_controller_frequency_ms;
	aocs_state.imu_task_running = false;
	aocs_state.measuring_semaphore_created = false;
	aocs_state.mode = AOCS_MTQ_OFF;
	aocs_state.breakage = false;

	PLATFORM_ss_is_ready(ss);

	PLATFORM_ss_expect_heartbeats(ss, HEARTBEAT_MAIN_TASK | HEARTBEAT_2nd_TASK);
	while (1) {
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode,
					aocs_state.current_controller_frequency_ms / portTICK_RATE_MS)) {

			prev_mode = PLATFORM_previous_satellite_mode();
			FUTURE_HOOK_3(aocs_mode_change, ss, &prev_mode, &mode);

			if (
					(prev_mode == SM_BOOTING &&
					mode == SM_INITIALIZING)     ||

					(prev_mode == SM_LOW_POWER // &&
					/* (mode == *) */ )) {

#ifdef AOCS_IMU_CHANNEL_OPENCLOSE
				imu_channel_openclose = true;
#else
				imu_channel_openclose = (prev_mode == SM_BOOTING && mode == SM_INITIALIZING);
#endif

			    if(nvram.aocs.run_sampler == AOCS_START_CDH_SAMPLER) {
			    	aocs_run_sampler(ss);
			    }

				aocs_state.mode = nvram.aocs.default_mode;
				aocs_hardware_on(ss, imu_channel_openclose);
				aocs_state.current_controller_frequency_ms = nvram.aocs.default_controller_frequency_ms;
				MTQ_DIPOLE_SURVIVAL_PERCENT = nvram.aocs.max_survival_dipole;

			} else if (
					/* (prev_mode == ANY_MODE ) && <--- not real code. */
					mode == SM_LOW_POWER) {

				to_mode_low_power(ss);

			} else {
				// Mode change that doesn't affect this particular subsystem
			}
			PLATFORM_ss_is_ready(ss);
		} else {
			// No mode change. Tick
			PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);

			switch (mode) {
			case SM_OFF:
			case SM_BOOTING:
			case SM_INITIALIZING:
				break;
			case SM_LOW_POWER:
				MTQ_DIPOLE_MAX_ALLOWED = 0; /* Just in case... MTQ should be off anyway */
				if(aocs_state.mode != AOCS_LOW_POWER) to_mode_low_power(ss);
				break;
			case SM_SURVIVAL:
				MTQ_DIPOLE_MAX_ALLOWED = MTQ_DIPOLE_MAX * MTQ_DIPOLE_SURVIVAL_PERCENT;
				if(aocs_state.mode == AOCS_LOW_POWER) to_mode_detumbling(ss);
				break;
			case SM_MISSION:
				MTQ_DIPOLE_MAX_ALLOWED = MTQ_DIPOLE_MAX;
				if(aocs_state.mode == AOCS_LOW_POWER) to_mode_detumbling(ss);
				break;
			case SM_COUNT:
				break;
			}

			if(!SATELLITE_MODE_WITH_AOCS(mode)) continue; /* go back to SS while (1) { */

			switch (aocs_state.mode) {
			case AOCS_MTQ_OFF:
				aocs_mtq_off();
				break;
			case AOCS_LOW_POWER:
				/* Don't check for return values in switches, I wouldn't know how to recover anyway */
				POWER_turnOn(POWER_SW_IMU);
				aocs_hardware_on(ss, AOCS_OPEN_CHANNEL_ON_LOW_POWER_AND_RESET);
				// turnOff and aocs_hardware_off are donde in to_mode_low_power.

				rv = adis1640x_read_burst(ch_adis, &(aocs_state.last_imu_raw));
				if (RV_SUCCESS != rv) aocs_state.last_imu_raw = imu_raw_error;

				css_read_vector();

				/* Always keep satellite in low power. This gives unrealiable telemetry
				 * because we may be using the IMU even if it is not passing its self test, or
				 * if it is in an incorrect configuration, but its better than no telemetry.
				 * Trying to recover the IMU from errors would consume even more power.
				 */
				to_mode_low_power(ss);

				break;
			case AOCS_DETUMBLING:
				/* If the measuring semaphore is not created we don't detumble */
				if(!aocs_state.measuring_semaphore_created) break;
				xSemaphoreTake(xSemaphore_measuring, 1000 / portTICK_RATE_MS);
				detumbling_step(ss);
				xSemaphoreGive(xSemaphore_measuring);
				break;
			case AOCS_POINTING:
				/* If the measuring semaphore is not created we don't do ponting */
				if(!aocs_state.measuring_semaphore_created) break;
				xSemaphoreTake(xSemaphore_measuring, 1000 / portTICK_RATE_MS);
				rv = pointing_step(ss);
				xSemaphoreGive(xSemaphore_measuring);

				/* Pointing needs array of data, may not be able to actuate */
				if(rv != RV_SUCCESS) to_mode_detumbling(ss);

				break;
			case AOCS_SPI_FAILURE:
			{
				/* SPI not working, possible next mode:
				 * 	-> AOCS_IMU_FAILURE: after recovery from SPI failure IMU must be checked.
				 */
				uint16_t device_id;

				/* Try resetting IMU, loop if it fails */
				rv = reset_imu(ss);
				if (RV_SUCCESS != rv) break;

				rv = adis1640x_read_register(ch_adis, ADIS16400_PRODUCT_ID, &device_id);
				if(RV_SUCCESS == rv && (IMU16400_DEVICE_ID_ENG == device_id
						                || IMU16400_DEVICE_ID_FLIGHT == device_id))
				{
					to_mode_imu_failure(RV_ERROR);
				}
				break;
			}
			case AOCS_IMU_FAILURE:
			{
				/* IMU not working, possible next mode:
				 * 	-> AOCS_DETUMBLING: after recovery from IMU failure steady state must be aquired.
				 */
				uint16_t diag_stat;
				uint16_t msc_ctrl;

				/* Try resetting IMU, loop if it fails */
				rv = reset_imu(ss);
				if (RV_SUCCESS != rv) break;

				/* With IMU already resetted */
				rv = check_imu_configuration();
				if (RV_SUCCESS != rv) rv = configure_imu();
				rv = check_imu_configuration();
				if (RV_SUCCESS == rv) {
					rv = adis1640x_read_register(ch_adis, ADIS16400_MSC_CTRL, &msc_ctrl);
					if (RV_SUCCESS != rv) break;

					rv = adis1640x_write_register(ch_adis, ADIS16400_MSC_CTRL, msc_ctrl | ADIS16400_MSC_CTRL_INT_SELF_TEST);
					if (RV_SUCCESS != rv) break;

					vTaskDelay(portTICK_RATE_MS / ADIS16400_SELF_TEST_DELAY);

					rv = adis1640x_read_register(ch_adis, ADIS16400_DIAG_STAT, &diag_stat);
					if (RV_SUCCESS != rv) break;
					/* TODO(peter) IMU may have low voltage, we don't know how this
					 * affects its performance. Check this and maybe accept low voltage
					 * as a valid mode for detumbling.
					 */
					if (0 != diag_stat) break;

					to_mode_detumbling(ss);
				}
				break;
			} /* case AOCS_IMU_FAILURE: { */
			} /* switch(aocs_state.mode) { */

		}
	}
}

retval_t AOCS_get_imu_temperature(int32_t *temperature_mC) {
	*temperature_mC = (LASTT_RAW * 140) + 25000;

	if (BROKEN_IMU(aocs_state.mode)) {
		*temperature_mC = 0xDEAD;
		return RV_ERROR;
	}

	return RV_SUCCESS;
}

retval_t AOCS_get_gyro_magnitude(float *gyroMag_ds) {
	retval_t rv;
	if (LASTW_RAW(x) == 0xdead || BROKEN_IMU(aocs_state.mode)) {
		*gyroMag_ds = 3.14f;
		rv = RV_ERROR;
	} else {
		*gyroMag_ds = sqrt(pow(LASTW(x),2)+pow(LASTW(y),2)+pow(LASTW(z),2));
		rv = RV_SUCCESS;
	}
	return rv;
}
/****************** COMMANDS **************/

static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    frame_put_s16(oframe, aocs_state.sun_data.sun_vector_bits.x);
    frame_put_s16(oframe, aocs_state.sun_data.sun_vector_bits.y);
    frame_put_s16(oframe, aocs_state.sun_data.sun_vector_bits.z);
	frame_put_s16(oframe, (int16_t)(LASTB_RAW(x)));
    frame_put_s16(oframe, (int16_t)(LASTB_RAW(y)));
    frame_put_s16(oframe, (int16_t)(LASTB_RAW(z)));
    frame_put_s16(oframe, (int16_t)(LASTW_RAW(x)));
    frame_put_s16(oframe, (int16_t)(LASTW_RAW(y)));
    frame_put_s16(oframe, (int16_t)(LASTW_RAW(z)));
    frame_put_s16(oframe, (int16_t)(LASTT_RAW));
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    frame_put_bits_by_4(oframe, (int8_t)(LASTW(x)*10), 8);
    frame_put_bits_by_4(oframe, (int8_t)(LASTW(y)*10), 8);
    frame_put_bits_by_4(oframe, (int8_t)(LASTW(z)*10), 8);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	char buf[20];
	float gyroMag_ds;

	frame_put_data(oframe, "Gyr:", 4);
	if(RV_SUCCESS == AOCS_get_gyro_magnitude(&gyroMag_ds)) {
		snprintf(buf, sizeof(buf), "%3.2fd/s ", gyroMag_ds);
	} else {
		snprintf(buf, sizeof(buf), "Err ");
	}
	return frame_put_data(oframe, buf, strlen(buf));
}

static retval_t cmd_set_detumbling_gain(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    float new_gain;

    rv = frame_get_f(iframe, &new_gain);
    if(rv != RV_SUCCESS) return rv;

   	nvram.aocs.Bxw_gain = new_gain;
   	MEMORY_nvram_save(&nvram.aocs.Bxw_gain, sizeof(nvram.aocs.Bxw_gain));

    return RV_SUCCESS;
}

static retval_t cmd_set_default_controller_frequency(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint32_t controller_frequency_ms;

    rv = frame_get_u32(iframe, &controller_frequency_ms);
    if(rv != RV_SUCCESS) return frame_put_u32(oframe, 0xefefefef);

    nvram.aocs.default_controller_frequency_ms = controller_frequency_ms;
    MEMORY_nvram_save(&nvram.aocs.default_controller_frequency_ms,
    		sizeof(nvram.aocs.default_controller_frequency_ms));

    return frame_put_u32(oframe, nvram.aocs.default_controller_frequency_ms);
}

static retval_t cmd_set_current_controller_frequency(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint32_t controller_frequency_ms;

    rv = frame_get_u32(iframe, &controller_frequency_ms);
    if(rv != RV_SUCCESS) return frame_put_u32(oframe, 0xefefefef);

    aocs_state.current_controller_frequency_ms = controller_frequency_ms;

    return frame_put_u32(oframe, aocs_state.current_controller_frequency_ms);
}

static retval_t cmd_get_mode(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;

    rv = frame_put_u32(oframe, aocs_state.mode);
    if(rv != RV_SUCCESS) return rv;

    return RV_SUCCESS;
}

static retval_t cmd_set_mode(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    aocs_mode_t new_mode;

    rv = frame_get_u32(iframe, &new_mode);
    if(rv != RV_SUCCESS) return rv;

    switch(new_mode) {
    case AOCS_IMU_FAILURE:
    	to_mode_imu_failure(RV_ERROR);
    	break;
    case AOCS_SPI_FAILURE:
    	to_mode_spi_failure(RV_ERROR);
    	break;
    case AOCS_LOW_POWER:
    	to_mode_low_power(self);
    	break;
    case AOCS_DETUMBLING:
    	to_mode_detumbling(self);
    	break;
    case AOCS_POINTING:
    	to_mode_pointing(self);
    	break;
    default:
    	aocs_state.mode = new_mode;
    }

    rv = frame_put_u32(oframe, aocs_state.mode);
    if(rv != RV_SUCCESS) return rv;

    return RV_SUCCESS;
}

static retval_t cmd_log_to_buffer(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint8_t new_state;

    rv = frame_get_u8(iframe, &new_state);
    if(rv != RV_SUCCESS) return rv;

    log_to_aocs_buffer = new_state;

    rv = frame_put_u8(oframe, log_to_aocs_buffer);
    if(rv != RV_SUCCESS) return rv;

    return RV_SUCCESS;
}

static retval_t cmd_get_buffer(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint32_t index=0, count=1;
    uint32_t i = index;

    rv = frame_get_u32(iframe, &index);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_get_u32(iframe, &count);
    if(rv != RV_SUCCESS) return rv;

    /* Anti segfault */
    if (index + count > aocs_circular_buffer_size) {
    	frame_put_u16(oframe, 0xf2f2);
    	return RV_SUCCESS;
    }

    frame_put_u16(oframe, 0xf1f1);

    for(i = index; i < index + count; i++) {
    	frame_put_u16(oframe, aocs_circular_buffer[i].power_supply_voltage);
    	frame_put_u16(oframe, aocs_circular_buffer[i].gyro_x);
    	frame_put_u16(oframe, aocs_circular_buffer[i].gyro_y);
    	frame_put_u16(oframe, aocs_circular_buffer[i].gyro_z);
    	frame_put_u16(oframe, aocs_circular_buffer[i].accel_x);
    	frame_put_u16(oframe, aocs_circular_buffer[i].accel_y);
    	frame_put_u16(oframe, aocs_circular_buffer[i].accel_z);
    	frame_put_u16(oframe, aocs_circular_buffer[i].mag_x);
    	frame_put_u16(oframe, aocs_circular_buffer[i].mag_y);
    	frame_put_u16(oframe, aocs_circular_buffer[i].mag_z);
    	frame_put_u16(oframe, aocs_circular_buffer[i].temp);
    	frame_put_u16(oframe, aocs_circular_buffer[i].adc);
    	frame_put_u16(oframe, 0xf1f1);
    }

    return RV_SUCCESS;
}

static retval_t cmd_get_adc(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {

  	frame_put_u16(oframe, aocs_state.sun_data.css_adc_measurement_volts.x_pos * (4095 / 3.3 /*volts*/ ));
  	frame_put_u16(oframe, aocs_state.sun_data.css_adc_measurement_volts.y_pos * (4095 / 3.3 /*volts*/ ));
  	frame_put_u16(oframe, aocs_state.sun_data.css_adc_measurement_volts.z_pos * (4095 / 3.3 /*volts*/ ));
  	frame_put_u16(oframe, aocs_state.sun_data.css_adc_measurement_volts.x_neg * (4095 / 3.3 /*volts*/ ));
  	frame_put_u16(oframe, aocs_state.sun_data.css_adc_measurement_volts.y_neg * (4095 / 3.3 /*volts*/ ));
  	frame_put_u16(oframe, aocs_state.sun_data.css_adc_measurement_volts.z_neg * (4095 / 3.3 /*volts*/ ));
  	frame_put_u16(oframe, aocs_state.adc_fix_scale * (4095 / 2 /* ratio, 1 = no fix */));

    return RV_SUCCESS;
}

static retval_t cmd_mtq_actuate(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint8_t new_value;

    rv = frame_get_u8(iframe, &new_value);
    if(rv != RV_SUCCESS) return rv;
    new_value = !new_value;

    /* Detumbling should turn off mtq prior to magnetometer
     * measurement in next cycle, put this here just in case.
     *
     * Don't turn on pwm in the switch from 1 to 0 because
     * we leave that to the control law.
     */
    if(nvram.aocs.no_mtq_actuate == 0 && new_value == 1) {
    	aocs_mtq_off();
    }

    nvram.aocs.no_mtq_actuate = new_value;

    return RV_SUCCESS;
}

static retval_t cmd_pwm_init(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    pwm_init();
    return RV_SUCCESS;
}

static retval_t cmd_pwm_enable(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint8_t channel;

    rv = frame_get_u8(iframe, &channel);
    if (RV_SUCCESS != rv) return rv;

	pwm_enable(channel);
    return RV_SUCCESS;
}

static retval_t cmd_pwm_disable(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint8_t channel;

    rv = frame_get_u8(iframe, &channel);
    if (RV_SUCCESS != rv) return rv;

	pwm_disable(channel);
    return RV_SUCCESS;
}

static retval_t cmd_pwm_set_duty(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint8_t channel;
    int16_t duty;

    rv = frame_get_u8(iframe, &channel);
    if (RV_SUCCESS != rv) return rv;

    rv = frame_get_s16(iframe, &duty);
    if (RV_SUCCESS != rv) return rv;

	pwm_set_duty(channel, duty);
    return RV_SUCCESS;
}

static retval_t cmd_breakage_key(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint16_t new_value;

    rv = frame_get_u16(iframe, &new_value);
    if(rv != RV_SUCCESS) return rv;

    if(new_value == 0x40c5) {
    	aocs_state.breakage = true;
    } else {
    	aocs_state.breakage = false;
	}

    rv = frame_put_u16(oframe, aocs_state.breakage);

    return rv;
}

static retval_t cmd_imu_read_register(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint16_t regnr, value;

    if(!aocs_state.breakage) {
    	rv = frame_put_u16(oframe, 0xcaca);
    	return rv;
    }

    rv = frame_get_u16(iframe, &regnr);
    if(rv != RV_SUCCESS) return rv;

	rv = adis1640x_read_register(ch_adis, regnr, &value);
	if (rv != RV_SUCCESS) return rv;

    rv = frame_put_u16(oframe, value);

    return rv;
}

static retval_t cmd_imu_write_register(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint16_t regnr, value;

    if(!aocs_state.breakage) {
    	rv = frame_put_u16(oframe, 0xcaca);
    	return rv;
    }

    rv = frame_get_u16(iframe, &regnr);
    if(rv != RV_SUCCESS) return rv;

    rv = frame_get_u16(iframe, &value);
    if(rv != RV_SUCCESS) return rv;

	rv = adis1640x_write_register(ch_adis, regnr, value);
	if (rv != RV_SUCCESS) return rv;

    return rv;
}

static retval_t cmd_set_magcal_matrix(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    uint8_t i, j;
    float value;

    if(!aocs_state.breakage) {
    	rv = frame_put_u32(oframe, 0xcaca);
    	rv = frame_put_u8(oframe, 0xca);
    	return rv;
    }

    rv = frame_get_u8(iframe, &i);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_get_u8(iframe, &j);
    if(rv != RV_SUCCESS) return rv;

    rv = frame_get_f(iframe, &value);
    if(rv != RV_SUCCESS) return rv;

    nvram.aocs.magcal_matrix_signed_raw[i][j] = value;
    MEMORY_nvram_save(&nvram.aocs.magcal_matrix_signed_raw,
    		sizeof(nvram.aocs.magcal_matrix_signed_raw));

    rv = frame_put_u32(oframe, (uint32_t)(
    		nvram.aocs.magcal_matrix_signed_raw[i][j]>0?1:-1 *
    		nvram.aocs.magcal_matrix_signed_raw[i][j] * 100000)
    		);
	if(rv != RV_SUCCESS) return rv;
    rv = frame_put_u8(oframe, nvram.aocs.magcal_matrix_signed_raw[i][j] < 0);
    if(rv != RV_SUCCESS) return rv;

    return rv;
}

static retval_t cmd_restart_sampler(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;

    if(!aocs_state.breakage) {
    	rv = frame_put_u8(oframe, 0xca);
    	rv = frame_put_u8(oframe, 0xca);
    	return rv;
    }

    rv = frame_put_u8(oframe, (uint8_t)nvram.aocs.run_sampler);
    if(rv != RV_SUCCESS) return rv;
    nvram.aocs.run_sampler = AOCS_START_CDH_SAMPLER;
    MEMORY_nvram_save(&nvram.aocs.run_sampler, sizeof(nvram.aocs.run_sampler));
    rv = frame_put_u8(oframe, (uint8_t)nvram.aocs.run_sampler);
    if(rv != RV_SUCCESS) return rv;

    return rv;
}

static retval_t cmd_configure_sampler(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;

    rv = frame_get_u16(iframe, &nvram.aocs.sampler_configuration.offset);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_get_u16(iframe, &nvram.aocs.sampler_configuration.size);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_get_u16(iframe, &nvram.aocs.sampler_configuration.count);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_get_u16(iframe, &nvram.aocs.sampler_configuration.interval);
    if(rv != RV_SUCCESS) return rv;

    MEMORY_nvram_save(&nvram.aocs.sampler_configuration, sizeof(nvram.aocs.sampler_configuration));

    rv = frame_put_u16(oframe, nvram.aocs.sampler_configuration.offset);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_put_u16(oframe, nvram.aocs.sampler_configuration.size);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_put_u16(oframe, nvram.aocs.sampler_configuration.count);
    if(rv != RV_SUCCESS) return rv;
    rv = frame_put_u16(oframe, nvram.aocs.sampler_configuration.interval);
    if(rv != RV_SUCCESS) return rv;

    return rv;
}


extern retval_t aocs_run_test(uint32_t test_number);

const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("sunvectorX:s16:[sunvectorX/16384.0],sunvectorY:s16:[sunvectorY/16384.0],sunvectorZ:s16:[sunvectorZ/16384.0],magnetometerX_mg:s16:[magnetometerX_mg*0.5],magnetometerY_mg:s16:[magnetometerY_mg*0.5],magnetometerZ_mg:s16:[magnetometerZ_mg*0.5],gyroX_mg:s16:[gyroX_mg*0.0125],gyroY_mg:s16:[gyroY_mg*0.0125],gyroZ_mg:s16:[gyroZ_mg*0.0125],temperature_IMU_C:s16:[(temperature_IMU_C*0.14)+25]", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_NO_MTQ_ACTUATE, cmd_mtq_actuate, "mtq", "MTQ actuate (1 = Actuate, 0 = Do nothing)", "actuate:u8", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_SET_DETUMBLING_GAIN, cmd_set_detumbling_gain, "setDetumbling", "Set gain in nvram", "gain:u32", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_GET_MODE, cmd_get_mode, "getMode", "Get AOCS mode 0: detumbling, 1:broken API, 2: broken IMU 3: low power 0xdead40c5: mtq off", "", "mode:u32"),
	DECLARE_COMMAND(SS_CMD_AOCS_SET_MODE, cmd_set_mode, "set", "Get AOCS mode 0: detumbling, 1:broken SPI, 2: broken IMU 3: low power 0xdead40c5: mtq off", "mode:u32", "mode:u32"),
	DECLARE_COMMAND(SS_CMD_AOCS_PWM_INIT, cmd_pwm_init, "pwm_init", "Initialize PWM driver","", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_PWM_ENABLE_CHANNEL, cmd_pwm_enable, "pwm_enable", "Enable one PWM channel <channel>","enable:u8", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_PWM_DISABLE_CHANNEL, cmd_pwm_disable, "pwm_disable", "Disable one PWM channel <channel>","channel:u8", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_PWM_SET_DUTY_CHANNEL, cmd_pwm_set_duty, "pwm_duty", "Set duty cycle for one PWM channel <channel> <signed duty>","channel:u8,duty:s16", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_SET_DEFAULT_CONTROLLER_FREQUENCY, cmd_set_default_controller_frequency, "setDefaultController", "Set controller frequency in ms","frequency:u32", "frequency:u32"),
	DECLARE_COMMAND(SS_CMD_AOCS_SET_CURRENT_CONTROLLER_FREQUENCY, cmd_set_current_controller_frequency, "setCurrentController", "Set controller frequency in ms","frequency:u32", "frequency:u32"),
	DECLARE_COMMAND(SS_CMD_AOCS_LOG_TO_BUFFER, cmd_log_to_buffer, "logToBuffer", "Log IMU bursts to circular buffer", "on:u8", "actual:u8"),
	DECLARE_COMMAND(SS_CMD_AOCS_GET_BUFFER, cmd_get_buffer, "getBuffer", "Read IMU circular buffer", "index:u32,count:u32", "result:u16[]"),
	DECLARE_COMMAND(SS_CMD_AOCS_GET_ADC, cmd_get_adc, "getCSSADC", "Read last ADC measurement", "", "result:u16[]"),
	DECLARE_COMMAND(SS_CMD_AOCS_CACA, cmd_breakage_key, "breakage", "Allow AOCS breakage", "key:u16", "result:u16"),
	DECLARE_COMMAND(SS_CMD_AOCS_IMU_READ, cmd_imu_read_register, "imuRead", "Read IMU register, needs breakage key", "register:u16", "result:u16"),
	DECLARE_COMMAND(SS_CMD_AOCS_IMU_WRITE, cmd_imu_write_register, "imuWrite", "Write IMU register, needs breakage key", "register:u16,value:u16", ""),
	DECLARE_COMMAND(SS_CMD_AOCS_SET_MAGCAL_MATRIX, cmd_set_magcal_matrix, "magcalMatrix", "Configure magnetometer calibration, needs breakage key", "row:u8, col:u8, floatData: u32", "savedFloatAsInt:u32,savedSign:u8"),
	DECLARE_COMMAND(SS_CMD_AOCS_RESTART_SAMPLER, cmd_restart_sampler, "resetSampler", "Reset sampler, needs breakage key", "", "prevValue:u8, actualValue:u8"),
	DECLARE_COMMAND(SS_CMD_AOCS_CONFIGURE_SAMPLER, cmd_configure_sampler, "configureSampler", "Configure AOCS sampler", "offset:u16,count:u16,times:u16,interval:u16", "offset:u16,size:u16,count:u16,interval:u16"),
};

static subsystem_api_t subsystem_api = {
    .main_task = &AOCS_main_task,
    .command_execute = &ss_command_execute,
};

extern const ss_tests_t aocs_tests;

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_AOCS,
    .usStackDepth = STACK_DEPTH_AOCS,
    .id = SS_AOCS,
    .name = "AOCS",
    .tests = &aocs_tests,
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_AOCS = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = &subsystem_state
};

