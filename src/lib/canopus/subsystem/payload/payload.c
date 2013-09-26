#include <canopus/frame.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/payload.h>
#include <canopus/subsystem/platform.h>
#include <canopus/subsystem/power.h>
#include <canopus/subsystem/cdh.h>
#include <canopus/nvram.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/commhub_1500.h>
#include <canopus/drivers/power/ina209.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "experiments.h"

static void initialize_channels() {
	retval_t rv;

    rv = channel_open(ch_svip_power);
    if (RV_SUCCESS != rv);

    rv = channel_open(ch_octopus_adc);
    if (RV_SUCCESS != rv);
}

retval_t PAYLOAD_transact_uart(uint8_t uart, frame_t *iframe, frame_t *oframe, bool lock, bool unlock) {
	retval_t rv;

	if (lock) {
		rv = channel_lock(ch_umbilical_out, 1000);
		SUCCESS_OR_RETURN(rv);
	}

	rv = commhub_connectUarts(COMMHUB_UART_ADDR_TMS_0_UART_0, uart);
	if (RV_SUCCESS != rv) {
		if (unlock) {
			channel_unlock(ch_umbilical_out);
		}
		return rv;
	}

	rv = channel_transact(ch_umbilical_out, iframe, 2000, oframe);

	(void)commhub_disconnectUartRx(uart);
	(void)commhub_connectUarts(COMMHUB_UART_ADDR_TMS_0_UART_0, COMMHUB_UART_ADDR_UMB);
	if (unlock) {
		channel_unlock(ch_umbilical_out);
	}

	return rv;
}

static void PAYLOAD_main_task(void *pvParameters) {
	satellite_mode_e mode = SM_BOOTING;
	satellite_mode_e prev_mode;

	subsystem_t * ss = (subsystem_t *)pvParameters;

	PLATFORM_ss_is_ready(ss);

	while (1) {
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode, PM_MODE_CHANGE_DETECT_TIME_SLOT_MS / portTICK_RATE_MS)) {
			prev_mode = PLATFORM_previous_satellite_mode();
			FUTURE_HOOK_3(payload_mode_change, ss, &prev_mode, &mode);

			if (prev_mode == SM_BOOTING && mode == SM_INITIALIZING) {
				initialize_channels();
			} else if (mode == SM_MISSION) {
				experiments_machine_tick(true);
			}
			PLATFORM_ss_is_ready(ss);
		} else {
			// No mode change. Tick
			PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);

	    	switch (mode) {
			case SM_OFF:
			case SM_BOOTING:
			case SM_INITIALIZING:
			case SM_SURVIVAL:
			case SM_LOW_POWER:
				break;
			case SM_MISSION:
				experiments_machine_tick(false);
				break;
			case SM_COUNT:
				break;
			}
		}
	}
}

#define NOTBASH_SIGNAL_IDLE			0
#define NOTBASH_RESYNCH_PARITY		0x70

retval_t PAYLOAD_svip_transact(frame_t *iframe, frame_t *oframe, bool lock, bool unlock) {
	frame_t *encodded;
	uint8_t inb;
	retval_t rv;
	size_t rx_count;

	rv = frame_allocate(&encodded);
	SUCCESS_OR_RETURN(rv);

	frame_put_u8(encodded, NOTBASH_SIGNAL_IDLE);
	frame_put_u8(encodded, NOTBASH_RESYNCH_PARITY);
	rx_count = 1;	/* The svip will not read NOTBASH_SIGNAL_IDLE */

	while (RV_SUCCESS == frame_get_u8(iframe, &inb)) {
		rx_count += 2;
		frame_put_u8(encodded, inb & 0x0F);
		rv = frame_put_u8(encodded, (inb >> 4) & 0x0F);
		if (RV_SUCCESS != rv) break;
	}

	/* complete to 32 bytes multiple, so the SVIP reads. It must be SVIP's UART or DMA buffer size */
	for(;(rx_count % 32) != 0;rx_count++) {
		frame_put_u8(encodded, NOTBASH_RESYNCH_PARITY);
	}

	frame_reset_for_reading(encodded);
	rv = PAYLOAD_transact_uart(COMMHUB_UART_ADDR_SVIP_0, encodded, oframe, true, true);
	frame_dispose(encodded);

	return rv;
}

/************************** COMMANDS ****************************/

static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_put_u16(oframe, nvram.payload.total_run);
	return frame_put_u16(oframe, nvram.payload.total_failed_count);
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return RV_SUCCESS;
}

static retval_t cmd_delay(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	retval_t rv;
	uint32_t ms;

	rv = frame_get_u32(iframe, &ms);
	if (RV_SUCCESS != rv) return rv;

	vTaskDelay(ms / portTICK_RATE_MS);
	return RV_SUCCESS;
}

static retval_t cmd_octopus_gpio_set(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint32_t value;
	retval_t rv;

	rv = frame_get_u32(iframe, &value);
	if (RV_SUCCESS != rv) return rv;

	frame_put_u8(oframe, 0);

	return commhub_writeRegister(COMMHUB_REG_GP_OUT, value);
}

static retval_t cmd_octopus_gpio_get(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint16_t value;
	retval_t rv;

	rv = commhub_readRegister(COMMHUB_REG_GP_IN, &value);
	if (RV_SUCCESS != rv) return rv;

	return frame_put_u16(oframe, value);
}

static retval_t cmd_octopus_adc_get(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	frame_t valueFrame = DECLARE_FRAME_SPACE(4);
	uint32_t value;
	retval_t rv;

	rv = channel_recv(ch_octopus_adc, &valueFrame);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&valueFrame);
	rv = frame_get_u32(&valueFrame, &value);
	if (RV_SUCCESS != rv) return rv;

	return frame_put_u32(oframe, value);
}

static retval_t cmd_uart_transact(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	retval_t rv;
	uint8_t uart;

	rv = frame_get_u8(iframe, &uart);
	SUCCESS_OR_RETURN(rv);

	return PAYLOAD_transact_uart(uart, iframe, oframe, true, true);
}

static retval_t cmd_uart_transact_long(const subsystem_t *self, frame_t * iframe, frame_t *oframe, uint32_t seqnum) {
	frame_t *cmd_frame;
	retval_t rv;
	uint8_t uart, timeout_s;
	bool lock, unlock;
	portTickType startTime;

	startTime = xTaskGetTickCount();

	(void)frame_get_u8(iframe, &uart);
	rv = frame_get_u8(iframe, &timeout_s);
	SUCCESS_OR_RETURN(rv);

	if (timeout_s > 0) timeout_s--;

	lock = !CDH_command_is_internal(iframe);
	unlock = (0 == timeout_s);

	rv = PAYLOAD_transact_uart(uart, iframe, oframe, lock, unlock);
	if (RV_SUCCESS == rv) {
		if (timeout_s > 0) {
			rv = CDH_command_new(&cmd_frame, seqnum, SS_PAYLOAD, SS_CMD_UART_TRANSACT_LONG);
			if (RV_SUCCESS == rv) {
				frame_put_u8(cmd_frame, uart);
				frame_put_u8(cmd_frame, timeout_s);
				rv = CDH_command_enqueue(cmd_frame);
			}
		}
	}

	if (RV_SUCCESS != rv) {
		if (!unlock) {
			// If we didn't unlock inside transact_uart()
			channel_unlock(ch_umbilical_out);
		}
	}

	vTaskDelayUntil(&startTime, 1000 / portTICK_RATE_MS);
	return rv;
}

retval_t cmd_commhub_register_read(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint8_t reg;
	uint16_t value;
	retval_t rv;

	rv = frame_get_u8(iframe, &reg);
	SUCCESS_OR_RETURN(rv);

	rv = commhub_readRegister(reg, &value);
	if (RV_SUCCESS == rv) {
		frame_put_u16(oframe, value);
	}
	return rv;
}

static
retval_t cmd_commhub_register_write(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint8_t reg;
	uint16_t value;
	retval_t rv;

	frame_get_u8(iframe, &reg);
	rv = frame_get_u16(iframe, &value);
	SUCCESS_OR_RETURN(rv);

	return commhub_writeRegister(reg, value);
}

static
retval_t cmd_start_experiment(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint16_t experiment;
	retval_t rv;

	rv = frame_get_u16(iframe, &experiment);
	SUCCESS_OR_RETURN(rv);

	return PAYLOAD_start_experiment(experiment, oframe);
}

static
retval_t cmd_end_experiment(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint16_t experiment;
	retval_t rv;

	rv = frame_get_u16(iframe, &experiment);
	SUCCESS_OR_RETURN(rv);

	return PAYLOAD_end_experiment(experiment, oframe);
}

retval_t cmd_get_last_results(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	return PAYLOAD_get_latest_experiment_results(oframe);
}

retval_t cmd_set_experiments_enabled(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint8_t enabled;
	retval_t rv;

	rv = frame_get_u8(iframe, &enabled);
	SUCCESS_OR_RETURN(rv);

	nvram.payload.experiments_enabled = enabled;
	return RV_SUCCESS;
}

retval_t cmd_next_experiment(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint8_t experiment;
	retval_t rv;

	rv = frame_get_u8(iframe, &experiment);
	SUCCESS_OR_RETURN(rv);

	nvram.payload.last_run_test = ((int16_t)experiment)-1;
	return RV_SUCCESS;
}

retval_t cmd_svip_transact(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	uint8_t experiment;
	retval_t rv;

	return PAYLOAD_svip_transact(iframe, oframe, true, true);
}

const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("experimentsRun:u16, experimentsFailed:u16", ""),
    DECLARE_COMMAND(SS_CMD_PAYLOAD_DELAY, cmd_delay, "delay", "Just a delay in milliseconds", "ms:u32", ""),
    DECLARE_COMMAND(SS_CMD_OCTOPUS_GPIO_SET, cmd_octopus_gpio_set, "gpio", "Just a delay in milliseconds", "set:u32", "ack:u8"),
    DECLARE_COMMAND(SS_CMD_OCTOPUS_GPIO_GET, cmd_octopus_gpio_get, "gpioGet", "Just a delay in milliseconds", "", "value:u32"),
    DECLARE_COMMAND(SS_CMD_OCTOPUS_ADC_GET, cmd_octopus_adc_get, "adcGet", "Just a delay in milliseconds", "", "value:u16"),
    DECLARE_COMMAND(SS_CMD_UART_TRANSACT, cmd_uart_transact, "transact", "Send a message to the UART (115200) and wait for some answer","uart:u8,data:str", "answer:str"),
    DECLARE_COMMAND(SS_CMD_UART_TRANSACT_LONG, cmd_uart_transact_long, "transact", "Send a message to the UART (115200) and wait for some answer","uart:u8,timeoutSeconds:u8,data:str", "answer:str"),
    DECLARE_COMMAND(SS_CMD_COMMHUB_REGISTER_READ, cmd_commhub_register_read, "commhubRead", "Read a register from the CommHub","register:u8", "value:u16"),
    DECLARE_COMMAND(SS_CMD_COMMHUB_REGISTER_WRITE, cmd_commhub_register_write, "commhubWrite", "Write a register to the CommHub","register:u8,value:u16", ""),
    DECLARE_COMMAND(SS_CMD_START_EXPERIMENT, cmd_start_experiment, "start", "Run an experiment on the Overo","experiment:u16", "answer:str"),
    DECLARE_COMMAND(SS_CMD_END_EXPERIMENT, cmd_end_experiment, "end", "Run an experiment on the Overo","experiment:u16", "success:u8,allAnswer:str"),
    DECLARE_COMMAND(SS_CMD_GET_LAST_RESULTS, cmd_get_last_results, "getLastResults", "Get results from the last experiment","", "isShorts:u8,experiment:u8,allAnswer:str"),
    DECLARE_COMMAND(SS_CMD_SET_EXPERIMENTS_ENABLED, cmd_set_experiments_enabled, "setExperiments", "Enable or disable experiments","enabled:u8", ""),
    DECLARE_COMMAND(SS_CMD_NEXT_EXPERIMENT, cmd_next_experiment, "next", "Enable or disable experiments","experiment:u8", ""),
    DECLARE_COMMAND(SS_CMD_SVIP_TRANSACT, cmd_svip_transact, "svip", "Enable or disable experiments","transact:str", "answer:str"),
};

static subsystem_api_t subsystem_api = {
    .main_task = &PAYLOAD_main_task,
    .command_execute = &ss_command_execute,
};

extern const ss_tests_t payload_tests;

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_PAYLOAD,
    .usStackDepth = STACK_DEPTH_PAYLOAD,
    .id = SS_PAYLOAD,
    .name = "PAYLOAD",
    .tests = &payload_tests,
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_PAYLOAD = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = &subsystem_state
};
