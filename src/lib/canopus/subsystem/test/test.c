#include <canopus/frame.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/platform.h>

#include <canopus/drivers/channel.h>
#include <canopus/drivers/leds.h>
#include <canopus/board/channels.h> // TODO remove
#include <canopus/logging.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#define TEST_SS_MODE_CHANGE_MS 400

static void
send_char(char c)
{
	frame_t f = DECLARE_FRAME_BYTES(c, '\r', '\n');

	channel_send(ch_umbilical_out, &f);
}

static void TEST_main_task(void *pvParameters) {
	satellite_mode_e mode = SM_BOOTING;

	subsystem_t * ss = (subsystem_t *)pvParameters;

	PLATFORM_ss_is_ready(ss);

	while (1) {
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode, TEST_SS_MODE_CHANGE_MS / portTICK_RATE_MS)) {
			PLATFORM_ss_is_ready(ss);
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
				break;
			case SM_COUNT:
				break;
			}
		}
	}
}

static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return RV_SUCCESS;
}

const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("", ""),
	DECLARE_COMMAND(SS_CMD_SS_START, cmd_get_telemetry_beacon, "no_cmd", "", "", ""),
};

static subsystem_api_t subsystem_api = {
    .main_task = &TEST_main_task,
    .command_execute = &ss_command_execute,
};

extern const ss_tests_t test_tests;

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_PAYLOAD,
    .usStackDepth = STACK_DEPTH_PAYLOAD,
    .id = SS_TEST,
    .name = "TEST",
    .tests = &test_tests,
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_TEST = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = &subsystem_state
};
