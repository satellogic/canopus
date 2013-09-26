#include <canopus/frame.h>
#include <string.h>
#include <canopus/subsystem/power.h>
#include <canopus/drivers/power/ina209.h>
#include <canopus/drivers/commhub_1500.h>
#include <canopus/board/channels.h>
#include <canopus/subsystem/payload.h>
#include <canopus/subsystem/platform.h>
#include <canopus/nvram.h>
#include "experiments.h"

/*
 * nvram:
 * 		latest_short_results
 * 		latest_short_results_offset
 * 		INTER_TEST_TIME_s
 */

retval_t experiment_r2r_sweep(frame_t *short_output, frame_t *long_output) {
	float meassurement;
	uint8_t r;
	retval_t rv, answer;

	answer = rv = ina209_getBusVoltage(ch_ina_pd3v3, &meassurement);
	if (RV_SUCCESS == rv) meassurement = meassurement * 1000.0;
	else meassurement = -1111;
	frame_put_s16(long_output, (int16_t)meassurement);

	rv = POWER_turnOn(POWER_SW_PD_3v3);
	if (RV_SUCCESS != rv) answer = rv;

	for (r = 0;r<16;r++) {
		if (r & 1) rv = POWER_turnOn(POWER_SW_PD_TESTER_A);
		else rv = POWER_turnOff(POWER_SW_PD_TESTER_A);
		if (RV_SUCCESS != rv) answer = rv;

		if (r & 2) rv = POWER_turnOn(POWER_SW_PD_TESTER_B);
		else rv = POWER_turnOff(POWER_SW_PD_TESTER_B);
		if (RV_SUCCESS != rv) answer = rv;

		if (r & 4) rv = POWER_turnOn(POWER_SW_PD_TESTER_C);
		else rv = POWER_turnOff(POWER_SW_PD_TESTER_C);
		if (RV_SUCCESS != rv) answer = rv;

		if (r & 8) rv = POWER_turnOn(POWER_SW_PD_TESTER_D);
		else rv = POWER_turnOff(POWER_SW_PD_TESTER_D);
		if (RV_SUCCESS != rv) answer = rv;

		vTaskDelay(1000 / portTICK_RATE_MS);
		rv = ina209_getCurrent(ch_ina_pd3v3, &meassurement);
		if (RV_SUCCESS != rv) {
			meassurement = -1111;
			answer = rv;
		}
		rv = frame_put_s16(long_output, (int16_t)meassurement);
		if (RV_SUCCESS != rv) answer = rv;
	}
	POWER_turnOff(POWER_SW_PD_3v3);
	POWER_turnOff(POWER_SW_PD_TESTER_A);
	POWER_turnOff(POWER_SW_PD_TESTER_B);
	POWER_turnOff(POWER_SW_PD_TESTER_C);
	POWER_turnOff(POWER_SW_PD_TESTER_D);
	return answer;
}

#define INTER_TEST_TIME_s	(10*60)

static uint8_t  latest_short_results[EXPERIMENT_RESULT_LATEST_COUNT*(EXPERIMENT_NUMBER_LENGTH+EXPERIMENT_RESULT_SHORT_LENGTH)];
static uint32_t latest_short_results_offset = 0;

static frame_t  latest_long_result_original = DECLARE_FRAME_SPACE(1+EXPERIMENT_RESULT_LONG_LENGTH);
static frame_t  latest_long_results;
static frame_t  latest_image_fragment = DECLARE_FRAME_SPACE(EXPERIMENT_RESULT_TOTAL_LENGTH);

struct experiment_t;

typedef retval_t experiment_start_t(const struct experiment_t *experiment, frame_t *oframe);
typedef retval_t experiment_stop_t(const struct experiment_t *experiment, frame_t *oframe);

static experiment_start_t experiment_start_overo;
static experiment_stop_t  experiment_stop_overo;
static experiment_start_t experiment_start_cmokery;
static experiment_stop_t  experiment_stop_cmokery;
static experiment_start_t experiment_start_svip;
static experiment_stop_t  experiment_stop_svip;
static experiment_start_t experiment_start_testing;
static experiment_stop_t  experiment_stop_testing;

typedef const struct experiment_type_t {
	experiment_start_t *start;
	experiment_stop_t *stop;
} experiment_type_t;

typedef const struct experiment_t {
	const uint8_t tests_per_suit;
	const experiment_type_t *type;
	const uint16_t run_time_s;
	const void *const cmd;
} experiment_t;

const experiment_type_t _testing = {experiment_start_testing, experiment_stop_testing};
const experiment_type_t _overo   = {experiment_start_overo, experiment_stop_overo};
const experiment_type_t _svip   =  {experiment_start_svip, experiment_stop_svip};
const experiment_type_t _cmokery = {experiment_start_cmokery, experiment_stop_cmokery};

static experiment_t experiments[] = {
		{3, &_cmokery, 0, (void*)SS_PLATFORM},
		{1, &_cmokery, 0, (void*)SS_CDH},
		{5, &_cmokery, 0, (void*)SS_POWER},
		{5, &_cmokery, 0, (void*)SS_AOCS},
//		{2, &_testing, 0, NULL},
		{1, &_overo, 5, "echo -ne \\\\x00`date -u +%d%m%H%M%S``uname -a` >/root/logs/tOut"},
		{1, &_overo, 5, "/root/gps/snd/gps-t1.sh\n"},
		{4, &_overo, 30, "2 0\n"},
//		{4, &_overo, 180, "2 1\n"},	/* replaced by subparts */
//		{3, &_overo, 300, "2 2\n"},	/* replaced by subparts */
		{1, &_svip, 5, "echo -ne \\\\x00`date -u +%d%m%H%M%S``uname -a` >/root/logs/tOut"},
		{1, &_overo, 40, "2 4\n"},		/* included in 2 1 */
		{1, &_overo, 30, "/root/memtest.sh\n"},
		{1, &_overo, 45, "2 5\n"},		/* included in 2 1 */
		{1, &_overo, 35, "/root/gps/snd/gps-t2.sh\n"},
		{1, &_overo, 40, "2 6\n"},		/* included in 2 1 */
		{1, &_overo, 70, "2 7\n"},		/* included in 2 1 */
		{1, &_overo, 30, "/root/gps/snd/gps-t3.sh\n"},
		{1, &_overo, 40, "2 8\n"},		/* included in 2 2 */
		{1, &_overo, 80, "2 9\n"},		/* included in 2 2 */
		{1, &_overo, 30, "2 19\n"},
		{1, &_overo, 330, "/root/gps/cleangps/gps-t4.sh\n"},
		{1, &_overo, 180, "2 10\n"},	/* included in 2 2 */
//		{1, &_overo, 720, "2 11\n"},	/* not recommended */
//		{1, &_overo, 180, "2 12\n"},	/* not recommended. uses too much power */
//		{1, &_overo, 2400, "2 13\n"},	/* not recommended. uses too much power */
//		{1, &_overo, 660, "2 14\n"},	/* uninteresting */
//		{2, &_overo, 1800, "2 15\n"},	/* useless in orbit. uses too much power. */
		{1, &_overo, 720, "2 16\n"},
		{1, &_overo, 330, "/root/gps/cleangps/gps-t5.sh\n"},
//		{1, &_overo, 720, "2 17\n"},	/* superseded by 2 16 */
//		{1, &_overo, 10, "2 18\n"},		/* will be run on get_one_more_image_fragment() */
		{1, &_overo, 660, "2 3\n"},
};

#define OVERO_BOOT_TIME_s				60
#define EXPERIMENT_RESULT_BEGIN_CHAR	'<'
#define CMD_GET_ONE_MORE_IMAGE_FRAGMENT	"2 18\n"

static retval_t experiment_overo_get_result(frame_t *oframe) {
	retval_t rv;
	frame_t cmd = DECLARE_FRAME_NONUL("echo -n \\<;echo -n \\<;cat /root/logs/tOut;sleep 5\n");
	frame_t *answer;
	uint8_t ch;

	rv = frame_allocate(&answer);
	SUCCESS_OR_RETURN(rv);

	do {
		rv = PAYLOAD_transact_uart(COMMHUB_UART_ADDR_OVERO_UART_1, &cmd, answer, true, true);
		if ((RV_SUCCESS != rv) && (RV_PARTIAL != rv)) {
			break;
		}

		frame_reset_for_reading(answer);
		ch = 0;
		do {
			while (RV_SUCCESS == frame_get_u8(answer, &ch)) {
				if (ch == EXPERIMENT_RESULT_BEGIN_CHAR) break;
			}
			rv = frame_get_u8(answer, &ch);
		} while ((RV_SUCCESS == rv) && (EXPERIMENT_RESULT_BEGIN_CHAR != ch));

		if (RV_SUCCESS != rv) {
			break;
		}

		rv = frame_transfer_count(oframe, answer, EXPERIMENT_RESULT_TOTAL_LENGTH);
	} while (0);
	frame_dispose(answer);

	if (RV_PARTIAL == rv) rv = RV_SUCCESS;
	return rv;
}

static retval_t get_one_more_image_fragment() {
	frame_t cmd = DECLARE_FRAME_NONUL(CMD_GET_ONE_MORE_IMAGE_FRAGMENT);
	retval_t rv;

	rv = PAYLOAD_transact_uart(COMMHUB_UART_ADDR_OVERO_UART_1, &cmd, NULL, true, true);
	SUCCESS_PARTIAL_OR_RETURN(rv);

	vTaskDelay(20*1000 / portTICK_RATE_MS);

	frame_reset(&latest_image_fragment);
	return experiment_overo_get_result(&latest_image_fragment);
}

static retval_t experiment_start_overo(experiment_t *experiment, frame_t *oframe) {
	retval_t rv;
	frame_t cmd = DECLARE_FRAME_SPACE(80);

	rv = POWER_turnOff(POWER_SW_SVIP);
	SUCCESS_OR_RETURN(rv);

	rv = POWER_turnOff(POWER_SW_CONDOR);
	SUCCESS_OR_RETURN(rv);

	rv = POWER_turnOn(POWER_SW_PD_12v);
	SUCCESS_OR_RETURN(rv);

	rv = POWER_turnOn(POWER_SW_OVERO);
	SUCCESS_OR_RETURN(rv);

	vTaskDelay(OVERO_BOOT_TIME_s * 1000 / portTICK_RATE_MS);

	get_one_more_image_fragment();

	rv = frame_put_data(&cmd, experiment->cmd, strlen(experiment->cmd));
	if (RV_SUCCESS != rv) {
		POWER_turnOff(POWER_SW_OVERO);
	} else {
		frame_reset_for_reading(&cmd);
		rv = PAYLOAD_transact_uart(COMMHUB_UART_ADDR_OVERO_UART_1, &cmd, oframe, true, true);
		if (RV_PARTIAL == rv) rv = RV_SUCCESS;
	}
	return rv;
}

static retval_t experiment_stop_overo(experiment_t *experiment, frame_t *oframe) {
	retval_t rv;

	rv = experiment_overo_get_result(oframe);

	(void /*best effort*/)POWER_turnOff(POWER_SW_PD_12v);
	(void /*best effort*/)POWER_turnOff(POWER_SW_OVERO);
	return rv;
}

#define SVIP_BOOT_TIME_s	30

static retval_t experiment_start_svip(experiment_t *experiment, frame_t *oframe) {
	retval_t rv;
	frame_t cmd = DECLARE_FRAME_SPACE(50);

	rv = POWER_turnOff(POWER_SW_CONDOR);
	SUCCESS_OR_RETURN(rv);

	rv = POWER_turnOn(POWER_SW_PD_12v);
	SUCCESS_OR_RETURN(rv);

	rv = POWER_turnOn(POWER_SW_SVIP);
	SUCCESS_OR_RETURN(rv);

	vTaskDelay(SVIP_BOOT_TIME_s * 1000 / portTICK_RATE_MS);

	rv = frame_put_data(&cmd, experiment->cmd, strlen(experiment->cmd));
	if (RV_SUCCESS != rv) {
		POWER_turnOff(POWER_SW_PD_12v);
		POWER_turnOff(POWER_SW_SVIP);
	} else {
		frame_reset_for_reading(&cmd);
		rv = PAYLOAD_svip_transact(&cmd, oframe, true, true);
		if (RV_PARTIAL == rv) rv = RV_SUCCESS;
	}
	return rv;
}

static retval_t experiment_stop_svip(experiment_t *experiment, frame_t *oframe) {
	retval_t rv;
	frame_t cmd = DECLARE_FRAME_NONUL("echo -n \\<;echo -n \\<;cat /root/logs/tOut;sleep 5\n");
	frame_t *answer;
	uint8_t ch;

	rv = frame_allocate(&answer);
	if (RV_SUCCESS != rv) {
		(void /*best effort*/)POWER_turnOff(POWER_SW_PD_12v);
		(void /*best effort*/)POWER_turnOff(POWER_SW_SVIP);
		return rv;
	}

	rv = PAYLOAD_svip_transact(&cmd, answer, true, true);

	(void /*best effort*/)POWER_turnOff(POWER_SW_PD_12v);
	(void /*best effort*/)POWER_turnOff(POWER_SW_SVIP);
	do {
		if ((RV_SUCCESS != rv) && (RV_PARTIAL != rv)) {
			break;
		}

		frame_reset_for_reading(answer);
		ch = 0;
		do {
			while (RV_SUCCESS == frame_get_u8(answer, &ch)) {
				if (ch == EXPERIMENT_RESULT_BEGIN_CHAR) break;
			}
			rv = frame_get_u8(answer, &ch);
		} while ((RV_SUCCESS == rv) && (EXPERIMENT_RESULT_BEGIN_CHAR != ch));

		if (RV_SUCCESS != rv) {
			break;
		}

		rv = frame_transfer_count(oframe, answer, EXPERIMENT_RESULT_TOTAL_LENGTH);
	} while (0);
	frame_dispose(answer);

	if (RV_PARTIAL == rv) rv = RV_SUCCESS;
	return rv;
}

static retval_t experiment_start_cmokery(experiment_t *experiment, frame_t *oframe) {
	return RV_SUCCESS;
}

static retval_t experiment_stop_cmokery(experiment_t *experiment, frame_t *oframe) {
	const subsystem_t *ss;
	uint8_t failed_count;

	ss = PLATFORM_get_subsystem((ss_id_e)(uintptr_t)(experiment->cmd));
	failed_count = SUBSYSTEM_run_tests(ss, 0);

	return frame_put_u8(oframe, failed_count);
}

static retval_t experiment_start_testing(experiment_t *experiment, frame_t *oframe) {
#define ANSWER "\x01start hola123456789012start18901234567890"
	return frame_put_data(oframe, ANSWER, sizeof(ANSWER));
#undef ANSWER
}

static retval_t experiment_stop_testing(experiment_t *experiment, frame_t *oframe) {
#define ANSWER "\x01sstop chau12345678901stop678901234567890"
	return frame_put_data(oframe, ANSWER, sizeof(ANSWER));
#undef ANSWER
}

static experiment_t *get_experiment(uint8_t number) {
	if (number >= ARRAY_COUNT(experiments)) {
		return NULL;
	}

	return &experiments[number];
}
static uint16_t get_experiment_run_time_s(uint8_t number) {
	experiment_t *experiment;
	experiment = get_experiment(number);
	if (NULL == experiment) return 0;
	return experiment->run_time_s;
}

static uint16_t get_experiment_tests_per_suit(uint8_t number) {
	experiment_t *experiment;
	experiment = get_experiment(number);
	if (NULL == experiment) return 0;
	return experiment->tests_per_suit;
}

retval_t PAYLOAD_start_experiment(uint16_t experiment_number, frame_t *oframe) {
	experiment_t *experiment;

	experiment = get_experiment(experiment_number);
	if (NULL == experiment) return RV_NOENT;

	return experiment->type->start(experiment, oframe);
}

retval_t PAYLOAD_end_experiment(uint16_t experiment_number, frame_t *oframe) {
	experiment_t *experiment;

	experiment = get_experiment(experiment_number);
	if (NULL == experiment) return RV_NOENT;

	return experiment->type->stop(experiment, oframe);
}

bool PAYLOAD_is_there_any_experiment_result() {
	return frame_hasEnoughData(&latest_long_results, 1);
}

retval_t PAYLOAD_get_latest_experiment_results(frame_t *oframe) {
	static bool shortResults = true;
	retval_t rv;

	if (!PAYLOAD_is_there_any_experiment_result()) return RV_NOENT;

	frame_put_u8(oframe, shortResults);
	if (shortResults) {
		rv = frame_put_data(oframe, latest_short_results, sizeof(latest_short_results));
	} else {
		rv = frame_transfer(oframe, &latest_long_results);
		frame_reset(&latest_long_results);
	}
	shortResults = !shortResults;
	return rv;
}

bool PAYLOAD_is_there_any_image_fragment() {
	return !frame_hasEnoughSpace(&latest_image_fragment, EXPERIMENT_RESULT_TOTAL_LENGTH);
}

retval_t PAYLOAD_get_latest_image_fragment(frame_t *oframe) {
	if (!PAYLOAD_is_there_any_image_fragment()) return RV_NOENT;

	frame_reset(&latest_image_fragment);
	frame_advance(&latest_image_fragment, EXPERIMENT_RESULT_FAILED_COUNT_LENGTH+EXPERIMENT_RESULT_SHORT_LENGTH);
	return frame_transfer(oframe, &latest_image_fragment);
}

static retval_t save_results(frame_t *result) {
	retval_t rv;
	uint8_t experiment, failed_count, total_experiments;

	rv = frame_get_u8(result, &experiment);
	rv = frame_get_u8(result, &failed_count);
	SUCCESS_OR_RETURN(rv);

	// save statistics
	total_experiments = get_experiment_tests_per_suit(experiment);
	nvram.payload.total_run += total_experiments;
	if (failed_count <= total_experiments) {
		nvram.payload.total_failed_count += failed_count;
	}

	// save short result
	latest_short_results[latest_short_results_offset++] = experiment;
	rv = frame_get_data(result, &latest_short_results[latest_short_results_offset], EXPERIMENT_RESULT_SHORT_LENGTH);
	latest_short_results_offset += EXPERIMENT_RESULT_SHORT_LENGTH;
	latest_short_results_offset %= sizeof(latest_short_results);

	// save long result
	frame_copy(&latest_long_results, &latest_long_result_original);
	frame_put_u8(&latest_long_results, experiment);
	frame_transfer(&latest_long_results, result);
	frame_reset_for_reading(&latest_long_results);

	MEMORY_nvram_save(&nvram.payload, sizeof(nvram.payload));
	return rv;
}

static frame_t  latest_results_original = DECLARE_FRAME_SPACE(1+EXPERIMENT_RESULT_TOTAL_LENGTH);
static frame_t  latest_results;

void experiments_machine_tick(bool just_changed_to_mission) {
	static uint32_t lastTime = 0;
	static bool first_in_run = false;
	static enum {
		S_IDLE,S_WAITING_START,S_STARTING,S_WAITING_END,S_ENDED,
	} state;

	retval_t rv;

	if (!nvram.payload.experiments_enabled) return;
	if (CDH_antenna_is_deployment_enabled()) return;

	if (just_changed_to_mission) {
		state = S_IDLE;
		first_in_run = true;
		return;
	}

	switch (state) {
		case S_IDLE:
			lastTime = PLATFORM_get_cpu_uptime_s();
			state = S_WAITING_START;
			break;
		case S_WAITING_START:
			if (first_in_run) {
				state = S_STARTING;
				first_in_run = false;
			}
			if (PLATFORM_get_cpu_uptime_s() - lastTime > INTER_TEST_TIME_s) {
				state = S_STARTING;
			}
			break;
		case S_STARTING:
			nvram.payload.last_run_test++;
			if (nvram.payload.last_run_test >= ARRAY_COUNT(experiments)) {
				nvram.payload.last_run_test = 0;
			}
			frame_copy(&latest_results, &latest_results_original);
			frame_put_u8(&latest_results, nvram.payload.last_run_test);
			rv = PAYLOAD_start_experiment(nvram.payload.last_run_test, &latest_results);
			if (RV_SUCCESS == rv) {
				frame_reset_for_reading(&latest_results);

				lastTime = PLATFORM_get_cpu_uptime_s();
				state = S_WAITING_END;
			} else {
				state = S_IDLE;
			}

			// Saved now in case this tests consumes too much energy and turns off the satellite
			MEMORY_nvram_save(&nvram.payload.last_run_test, sizeof(nvram.payload.last_run_test));
			break;
		case S_WAITING_END:
			if (PLATFORM_get_cpu_uptime_s() - lastTime > get_experiment_run_time_s(nvram.payload.last_run_test)) {
				state = S_ENDED;
			}
			break;
		case S_ENDED:
			frame_copy(&latest_results, &latest_results_original);
			frame_put_u8(&latest_results, nvram.payload.last_run_test);
			rv = PAYLOAD_end_experiment(nvram.payload.last_run_test, &latest_results);
			if (RV_SUCCESS == rv) {
				frame_reset_for_reading(&latest_results);
			}

			save_results(&latest_results);
			state = S_IDLE;
			break;
	}
}
