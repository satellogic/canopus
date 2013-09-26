#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/logging.h>

#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/platform.h>
#include <canopus/subsystem/aocs/aocs.h>
#include <canopus/subsystem/cdh.h>
#include <canopus/subsystem/mm.h>
#include <canopus/subsystem/power.h>
#include <canopus/subsystem/payload.h>
#include <canopus/subsystem/thermal.h>
#include <canopus/nvram.h>
#include <cmockery.h>

#include <string.h>

extern subsystem_t SUBSYSTEM_TEST;

const subsystem_t *const subsystems[SS_MAX] = {
	// The order here DOES NOT set the initialization order, rather ss_id_e does.
	ARRAY_INITIALIZE_DEFAULT_VALUE(SS_MAX, subsystem_t *),
    [SS_PLATFORM] = &SUBSYSTEM_PLATFORM,
    [SS_MEMORY] = &SUBSYSTEM_MEMORY,
    [SS_POWER] = &SUBSYSTEM_POWER,
    [SS_THERMAL] = &SUBSYSTEM_THERMAL,
    [SS_CDH] = &SUBSYSTEM_CDH,
    [SS_AOCS] = &SUBSYSTEM_AOCS,
    [SS_PAYLOAD] = &SUBSYSTEM_PAYLOAD,
    [SS_TEST] = &SUBSYSTEM_TEST,
};

retval_t ss_command_execute(subsystem_t * self, frame_t * iframe, frame_t * oframe, uint32_t sequence_number) {
	ss_command_handler_t handler;
	uint8_t cmd;

	if (RV_SUCCESS != frame_get_u8(iframe, &cmd)) return RV_ILLEGAL;
    assert(NULL != self);
    assert(NULL != self->config);
	if (cmd >= self->config->command_handlers_count) {
        log_report_fmt(LOG_ALL, "%s: invalid cmd 0x%02x\n", self->config->name, cmd);
        return RV_ILLEGAL;
    }
	handler = self->config->command_handlers[cmd];
	if ((NULL == handler) || (INVALID_PTR == handler)) {
        log_report_fmt(LOG_ALL, "%s: no handler for cmd 0x%02x\n", self->config->name, cmd);
        return RV_ILLEGAL;
    }
	return handler(self, iframe, oframe, sequence_number);
}

retval_t ss_cmd_get_name(const struct subsystem_t *self, frame_t *iframe, frame_t *oframe) {
    assert(NULL != self);
	return frame_put_data(oframe, self->config->name, strlen(self->config->name));
}

int SUBSYSTEM_run_tests(const subsystem_t *ss, uint32_t test_number) {
	int rv;
	if (test_number >= ss->config->tests->count) {
		rv = RV_ILLEGAL;
	}
	if (test_number) {
		rv = _run_tests(&ss->config->tests->tests[test_number-1], 1);
	} else {
		rv = _run_tests(ss->config->tests->tests, ss->config->tests->count);
	}
	return rv;
}

/*********************** COMMANDS for all subsystems **********************/

retval_t ss_get_telemetry_beacon(const struct subsystem_t *ss, frame_t *oframe, uint32_t seqnum) {
	frame_t iframe = DECLARE_FRAME_BYTES(SS_CMD_GET_TELEMETRY_BEACON);

    assert(NULL != ss);
	return ss->api->command_execute(ss, &iframe, oframe, seqnum);
}

retval_t ss_get_telemetry_beacon_short(const struct subsystem_t *ss, frame_t *oframe, uint32_t seqnum) {
	frame_t iframe = DECLARE_FRAME_BYTES(SS_CMD_GET_TELEMETRY_BEACON_SHORT);

    assert(NULL != ss);
	return ss->api->command_execute(ss, &iframe, oframe, seqnum);
}

retval_t ss_get_telemetry_beacon_ascii(const struct subsystem_t *ss, frame_t *oframe, uint32_t seqnum) {
	frame_t iframe = DECLARE_FRAME_BYTES(SS_CMD_GET_TELEMETRY_BEACON_ASCII);

    assert(NULL != ss);
	return ss->api->command_execute(ss, &iframe, oframe, seqnum);
}

retval_t ss_cmd_run_test(subsystem_t *ss, frame_t * iframe, frame_t * oframe) {
	retval_t rv;
	uint32_t test_number;

	rv = frame_get_u32(iframe, &test_number);
	if (RV_SUCCESS != rv) return rv;

	rv = SUBSYSTEM_run_tests(ss, test_number);
	frame_put_u8(oframe, rv);
	return rv;
}


void ss_report_error(const struct subsystem_t *ss, ss_error_e error, char *filename, uint32_t line) {
    static const char *const error_s[] = {
        [SS_CRITICAL_ERR]   = "CRITICAL",
        [SS_FATAL_ERR]      = "FATAL",
    };

	ss->state->status = SS_ST_FATAL_ERROR;
	ss->state->status_arg_p   = filename;
	ss->state->status_arg_u32 = line;

    log_report_fmt(LOG_GLOBAL, "%s ERROR at %s:%u (%s)\n", error_s[error], filename, line, ss->config->name);
    if (SS_FATAL_ERR == error) {
	// Fixme: FATAL ERROR! WHAT TO DO?
/*	*(int*)-1 = 0;
	while (1);
	*/
    }
}
