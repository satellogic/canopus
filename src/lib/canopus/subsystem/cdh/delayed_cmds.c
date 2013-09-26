#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/cdh.h>

#include <canopus/frame.h>

static portTickType delayed_cmds_next_due_time;
static frame_t *delayed_cmds_next_cmd;
static xTaskHandle delayed_cmd_task;

retval_t CDH_command_enqueue_delayed(uint32_t delay_s, frame_t *cmd) {
	frame_t *old_cmd;
	portTickType old_due_time, due_time;
	retval_t rv;

	rv = RV_SUCCESS;
	due_time  = xTaskGetTickCount();
	due_time += delay_s * 1000 * portTICK_RATE_MS;
	if ((NULL == delayed_cmds_next_cmd) || (delayed_cmds_next_due_time > due_time)) {
		old_due_time = delayed_cmds_next_due_time;
		old_cmd = delayed_cmds_next_cmd;

		delayed_cmds_next_cmd = cmd;
		delayed_cmds_next_due_time = due_time;

		FUTURE_HOOK_3(cdh_enqueue_delayed_new_is_sooner, &old_cmd, &old_due_time, &rv);
		if (NULL != old_cmd) {
			frame_dispose(old_cmd);
			rv = RV_PARTIAL;
		}
	} else {
		rv = RV_ERROR;
	}
	return rv;
}

void cdh_delayed_cmd_task(void *arg) {
	portTickType now;
	while (1) {
		now = xTaskGetTickCount();

		if ((NULL != delayed_cmds_next_cmd) && (now >= delayed_cmds_next_due_time)) {
			CDH_command_enqueue(delayed_cmds_next_cmd);
			delayed_cmds_next_cmd = NULL;
			delayed_cmds_next_due_time = 0;
		}

		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void cmd_delayed_init() {
	delayed_cmds_next_cmd = NULL;
	delayed_cmds_next_due_time = 0;

	(void)xTaskCreate(
			&cdh_delayed_cmd_task,
			(int8_t*)"CDH/DelayedCmds",
			configMINIMAL_STACK_SIZE + 100,
			NULL,
			TASK_PRIORITY_SUBSYSTEM_CDH,
    		&delayed_cmd_task);
}

retval_t cmd_delayed_add(const subsystem_t *self, frame_t *iframe, frame_t *oframe, uint32_t sequence_number) {
	uint32_t delay_s;
	uint8_t ss;
	uint8_t cmd;
	frame_t *delayed_cmd;
	retval_t rv;

	if (RV_SUCCESS != frame_get_u32(iframe, &delay_s)) return RV_ERROR;
	if (RV_SUCCESS != frame_get_u8(iframe, &ss)) return RV_ERROR;
	if (RV_SUCCESS != frame_get_u8(iframe, &cmd)) return RV_ERROR;

	if (RV_SUCCESS != CDH_command_new(&delayed_cmd, sequence_number, ss, cmd)) return RV_NOSPACE;

	frame_transfer(delayed_cmd, iframe);
	rv = CDH_command_enqueue_delayed(delay_s, delayed_cmd);
	if (RV_SUCCESS != rv) frame_dispose(delayed_cmd);
	return rv;
}

retval_t cmd_delayed_list(const subsystem_t *ss, frame_t *iframe, frame_t *oframe) {
	retval_t rv = RV_SUCCESS;
	FUTURE_HOOK_3(cmd_delayed_list, iframe, oframe, &rv);
	return rv;
}

retval_t cmd_delayed_discard(const subsystem_t *ss, frame_t *iframe, frame_t *oframe) {
	retval_t rv = RV_SUCCESS;
	FUTURE_HOOK_3(cmd_delayed_discard, iframe, oframe, &rv);
	return rv;
}

