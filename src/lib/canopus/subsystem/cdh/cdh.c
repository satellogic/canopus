#include <canopus/assert.h>
#include <canopus/logging.h>
#include <canopus/board/channels.h>
#include <canopus/md5.h>
#include <canopus/cpu.h> // reset
#include <canopus/watchdog.h>

#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/cdh.h>
#include <canopus/subsystem/command.h>
#include <canopus/subsystem/payload.h>
#include <canopus/subsystem/aocs/aocs.h>
#include <canopus/nvram.h>
#include <canopus/drivers/radio/aprs.h>

#include <string.h>

#include "antenna_cmds.h"
#include "radio_cmds.h"
#include "antenna_deployment.h"
#include "delayed_cmds.h"

static portTickType next_beacon_enable_time_ticks = 0;

static retval_t configure_radio();
static void initialize_beacon();

retval_t CDH_delay_beacon(uint32_t seconds) {
	retval_t rv;
	portTickType now;

	if (seconds > CDH_BEACON_MAX_DELAY_S) seconds = CDH_BEACON_MAX_DELAY_S;

	now = xTaskGetTickCount();
	next_beacon_enable_time_ticks  = seconds * 1000 / portTICK_RATE_MS;
	next_beacon_enable_time_ticks += now;
	if (next_beacon_enable_time_ticks < now) {
		/* It has wrapped, don't allow it to */
		next_beacon_enable_time_ticks = portMAX_DELAY;
	} else {
		rv = RV_ILLEGAL;
	}
	return rv;
}

bool CDH_command_is_internal(frame_t *cmd) {
	return 0 != (cmd->flags & FRAME_FLAG_IS_TRUSTED);
}
retval_t CDH_command_new(frame_t **cmd_frame, uint32_t sequence_number, uint8_t ss, uint8_t cmd) {
	retval_t rv;

	rv = frame_allocate(cmd_frame);
	if (RV_SUCCESS == rv) {
		(*cmd_frame)->flags |= FRAME_FLAG_IS_TRUSTED;

		frame_put_u24(*cmd_frame, 0);	// NO MAC
		frame_put_u24(*cmd_frame, sequence_number);
		frame_put_u8(*cmd_frame, ss);
		rv = frame_put_u8(*cmd_frame, cmd);
	}
	return rv;
}
retval_t CDH_command_enqueue(frame_t *cmd_frame) {
	retval_t lithium_push_incoming_data(frame_t *frame);

	frame_reset_for_reading(cmd_frame);
	return lithium_push_incoming_data(cmd_frame);
}
retval_t CDH_radio_reset_and_reconfigure() {
	return configure_radio();
}

typedef retval_t cdh_hard_command_t(frame_t *iframe);

portTickType	last_received_command_time = 0;

static void FDIR_CDH_save_last_received_command_time() {
	last_received_command_time = xTaskGetTickCount();
}

static void FDIR_CDH_check_last_received_command_time() {
#	define ELAPSED_TICKS_SINCE(_start_)	(xTaskGetTickCount()-(_start_))

	if (ELAPSED_TICKS_SINCE(last_received_command_time) > nvram.cdh.FDIR_CDH_LAST_COMMAND_TIMEOUTs / portTICK_RATE_MS * 1000) {
		FDIR_CDH_save_last_received_command_time();
        if (nvram.cdh.FDIR_CDH_last_command_timeout_mask & FDIR_CDH_LAST_COMMAND_TIMEOUT_MASK_RESET_RADIO) {
            CDH_radio_reset_and_reconfigure();
        }
        if (nvram.cdh.FDIR_CDH_last_command_timeout_mask & FDIR_CDH_LAST_COMMAND_TIMEOUT_MASK_RESET_ALL) {
        	PLATFORM_safe_reset("No commands for long time");
        }
	}
#	undef ELAPSED_TICKS_SINCE
}

retval_t cdh_hc_reset(frame_t *iframe) {
	cpu_reset("Hard command");
}

retval_t cdh_hc_disable_watchdog(frame_t *iframe) {
    watchdog_disable();
    return RV_SUCCESS;
}

retval_t cdh_hc_use_XiP_fw(frame_t *iframe) {
	uint8_t XiPindex;
	void (*fn_XiP)();

	XiPindex = frame_get_u8_nocheck(iframe);
	iframe->position--;

	switch (XiPindex) {
		case 4:
			fn_XiP = (void(*)())0x48340000; // FIXME phil linkscript...
			break;
		case 8:
			fn_XiP = (void(*)())0x48380000;
			break;
		default:
			return RV_ILLEGAL;
	}
	fn_XiP();

    return RV_SUCCESS;
}

static const struct {
	cdh_hard_command_t *handler;
	const uint8_t hash[MD5_DIGEST_SIZE];
} cdh_hard_commands[] = {
		{&cdh_hc_reset, 				"\xa5\xc4\x3b\x59\x43\x17\x9f\xcc\x6c\xd7\x41\x4d\x65\x5e\xab\x25"},
		{&cdh_hc_disable_watchdog,		"\x70\x60\x51\x94\x01\x32\x2f\x28\xc5\x2e\xd7\x74\xaf\xc0\x92\xb0"},
		{&cdh_hc_use_XiP_fw,			"\x46\x31\x2b\x1d\x65\x98\x82\x9d\x5e\x90\x65\x06\x6c\x1b\x1b\x1d"},
};

#define CDH_PACKET_HARDCMD_SIZE ((1<<5)-1) /* prime number */
retval_t CDH_process_hard_commands(frame_t *cmd_frame) {
	MD5_CTX ctx;
	uint8_t *data;
	size_t len;
    int i;

	data = frame_get_data_pointer_nocheck(cmd_frame, 1);
	len  = _frame_available_data(cmd_frame);

    if (len != CDH_PACKET_HARDCMD_SIZE) return RV_ILLEGAL;

	MD5Init(&ctx);
	MD5Update(&ctx, data+CDH_PACKET_HEADER_SIZE, len-CDH_PACKET_HEADER_SIZE);
	MD5Final(&ctx);

    for (i = 0; i < ARRAY_COUNT(cdh_hard_commands); i++) {
        if (!memcmp(ctx.digest, cdh_hard_commands[i].hash, sizeof(ctx.digest))) {
            log_report_fmt(LOG_ALL, "CDH: processing hard-command #%d\n", i);
            return cdh_hard_commands[i].handler(cmd_frame);
        }
    }

	return RV_ILLEGAL;
}

void CDH_command_watchdog_kick() {
	subsystem_t *ss = PLATFORM_get_subsystem(SS_CDH);
	if (NULL != ss) {
		PLATFORM_ss_is_alive(ss, HEARTBEAT_2nd_TASK);
	}
}

static retval_t cdh_dispatch_commands(frame_t *iframe, frame_t *oframe) {
	ss_id_e ss_id;
	retval_t rv;
	const subsystem_t *ss;
	uint32_t mac, sequence_number = 0xFFFFFF;

#ifdef LOG_RAW_INCOMING
	frame_t aux_to_log;
	uint8_t b;

	frame_copy(&aux_to_log, iframe);
	log_report(LOG_SS_CDH, "Received from Radio: ");
	Console_setPrompt("");
	while (RV_SUCCESS == frame_get_u8(&aux_to_log, &b)) {
		log_report_fmt(LOG_SS_CDH, "%02x ", b);
	}
	frame_reset(&aux_to_log);
	log_report(LOG_SS_CDH, " (");
	while (RV_SUCCESS == frame_get_u8(&aux_to_log, &b)) {
		log_report_fmt(LOG_SS_CDH, "%c", b);
	}
	log_report(LOG_SS_CDH, ")\n");
	Console_setPrompt("Debug> ");
#endif

	if (!frame_hasEnoughData(iframe, CDH_PACKET_HEADER_SIZE)) {
		if (nvram.cdh.APRS_eanbled) {
			return APRS_process_incomming(iframe, oframe);
		} else {
			frame_put_u24(oframe, sequence_number);
			frame_put_u8(oframe, CDH_ANS_TESTING_NO_SPACE);
			return RV_NOSPACE;
		}
	}

	mac = frame_get_u24_nocheck(iframe);
	sequence_number = frame_get_u24_nocheck(iframe);

    frame_put_u24(oframe, sequence_number);

	if (!FRAME_IS_TRUSTED(iframe)) {
        /* external pkt */

		if (sequence_number <= nvram.cdh.last_seen_sequence_number) {
            log_report_fmt(LOG_SS_CDH, "CDH: out of sequence (got %d, expected %d)\n", sequence_number, nvram.cdh.last_seen_sequence_number);
			frame_put_u8(oframe, CDH_ANS_TESTING_BAD_SEQUENCE);
			return RV_NOENT;
		}

		if (RV_SUCCESS != frame_verify_mac(
				iframe,
				sequence_number,
				nvram.cdh.telecommand_key,
				sizeof(nvram.cdh.telecommand_key),
				mac)) {
            log_report_fmt(LOG_SS_CDH, "CDH: got untrusted frame #%d\n", sequence_number);
			frame_put_u8(oframe, CDH_ANS_TESTING_BAD_MAC);
			return RV_ILLEGAL;
		}

		if (sequence_number <= CDH_MAX_SEQUENCE_NUMBER) nvram.cdh.last_seen_sequence_number = sequence_number;
		else nvram.cdh.last_seen_sequence_number = 0;
		FDIR_CDH_save_last_received_command_time();
		if (nvram.cdh.persist_sequence_number) {
			MEMORY_nvram_save(&nvram.cdh.last_seen_sequence_number, sizeof(nvram.cdh.last_seen_sequence_number));
		}
	}

    while (frame_hasEnoughData(iframe, 1)) {
		ss_id = (ss_id_e)frame_get_u8_nocheck(iframe);

		if (ss_id >= ARRAY_COUNT(subsystems) || !IS_PTR_VALID(ss)) {
            log_report_fmt(LOG_SS_CDH, "CDH: invalid subsystem 0x%02x for frame #%d\n", ss_id, sequence_number);
			frame_put_u8(oframe, CDH_ANS_TESTING_BAD_SS);
			return RV_NOENT;
		}

		ss = subsystems[ss_id];
		rv = ss->api->command_execute(ss, iframe, oframe, sequence_number);
		if (RV_SUCCESS != rv) {
			frame_put_u8(oframe, rv);
			break;
		}
	}
	return RV_SUCCESS;
}

#define CDH_MINIMUM_FRAME_TRAILER_SPACE 50

static void CDH_rx_task(void *pvParameters) {
	retval_t rv;

	frame_t *iframe, *oframe;
	static uint8_t aux_frame_buf[MAX_FRAME_SIZE];
	static frame_t aux_frame     = DECLARE_FRAME(aux_frame_buf);
	uint32_t original_size;

	while (1) {
		/* If a command takes too long, the watchdog will reset the system */
		CDH_command_watchdog_kick();
		rv = lithium_recv_data(&iframe);
		if (RV_SUCCESS != rv) continue;

		rv = frame_allocate(&oframe);
		if (RV_SUCCESS != rv) {
			oframe = &aux_frame;
		}

		CDH_command_watchdog_kick();

		original_size = oframe->size;
		oframe->size -= CDH_MINIMUM_FRAME_TRAILER_SPACE;
		(void)cdh_dispatch_commands(iframe, oframe);
		oframe->size = original_size;

		frame_dispose(iframe);
		frame_reset_for_reading(oframe);
		if (frame_hasEnoughData(oframe, 4)) {
			/* at least the sequence number and a byte */

			/* allow for the receiving end to switch from TX to RX */
			vTaskDelay(nvram.cdh.command_response_delay_ms / portTICK_RATE_MS);

			lithium_send_data(oframe);
		} else {
			frame_dispose(oframe);
		}
		if (oframe == &aux_frame) {
			vTaskDelay(1000 / portTICK_RATE_MS);
			frame_reset(oframe);
		}
	}
}

static void cdh_update_beacon(frame_t *data) {
	ss_id_e i;

	frame_put_u24(data, CDH_SEQUENCE_NUMBER_BEACON);
	for (i=0; i < ARRAY_COUNT(subsystems); i++) {
        if (!IS_PTR_VALID(subsystems[i])) continue;
		ss_get_telemetry_beacon(subsystems[i], data, CDH_SEQUENCE_NUMBER_BEACON);
	}
}

#ifdef SHORT_BEACONS
static void cdh_update_beacon_short(frame_t *data) {
	unsigned int i;

	frame_put_u24(data, CDH_SEQUENCE_NUMBER_BEACON_SHORT);
	for (i=0; i < ARRAY_COUNT(subsystems); i++) {
        if (!IS_PTR_VALID(subsystems[i])) continue;
		ss_get_telemetry_beacon_short(subsystems[i], data, CDH_SEQUENCE_NUMBER_BEACON_SHORT);
	}
}
#endif // SHORT_BEACONS

static void cdh_update_beacon_ascii(frame_t *data) {
	char aprs_email_hdr[] = ":EMAIL    :manolito@satellogic.com ";
	unsigned int i;

	frame_put_data(data, aprs_email_hdr, sizeof(aprs_email_hdr)-1);
	for (i=0; i < ARRAY_COUNT(subsystems); i++) {
        if (!IS_PTR_VALID(subsystems[i])) continue;
		ss_get_telemetry_beacon_ascii(subsystems[i], data, CDH_SEQUENCE_NUMBER_BEACON_ASCII);
	}
}

static retval_t cdh_update_beacon_image_fragment(frame_t *data) {
	frame_put_u24(data, CDH_SEQUENCE_NUMBER_IMAGE_FRAGMENT);

	return PAYLOAD_get_latest_image_fragment(data);
}

static retval_t cdh_update_beacon_results(frame_t *data) {
	frame_put_u24(data, CDH_SEQUENCE_NUMBER_TEST_RESULTS);

	return PAYLOAD_get_latest_experiment_results(data);
}

static void beacon_update_task(void *pvParameters) {
	static int ascii_every_n = 0;
	subsystem_t *ss = pvParameters;
	frame_t beacon = DECLARE_FRAME_SPACE(MAX_BEACON_SIZE);
	float battery_v;
	retval_t rv;
	portTickType xLastWakeTime;
	portTickType now;
	uint32_t beaconDelayAccordingToBatteryVoltage_ms;
	satellite_mode_e currentMode;

	/* Wait until satellite is initialized */
	while (1) {
		currentMode = PLATFORM_current_satellite_mode();
		if (currentMode == SM_SURVIVAL) break;
		if (currentMode == SM_MISSION) break;
		if (currentMode == SM_LOW_POWER) break;
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

	initialize_beacon();

	xLastWakeTime = xTaskGetTickCount();

	while (1) {
		PLATFORM_ss_is_alive(ss, HEARTBEAT_3rd_TASK);
		now = xTaskGetTickCount();
		if (next_beacon_enable_time_ticks && (next_beacon_enable_time_ticks > now)) {
			vTaskDelay(1000 / portTICK_RATE_MS);
			xLastWakeTime = xTaskGetTickCount();
			continue;
		} else {
			next_beacon_enable_time_ticks = 0;
		}

		ascii_every_n++;
		ascii_every_n %= CDH_BEACON_ASCII_EVERY;

		beacon.size = MAX_BEACON_SIZE;
		frame_reset(&beacon);
		if (nvram.payload.experiments_enabled && CDH_BEACON_IS_IMAGE(ascii_every_n) && PAYLOAD_is_there_any_image_fragment()) {
	    	cdh_update_beacon_image_fragment(&beacon);
            frame_reset_for_reading(&beacon);
		} else
		if (nvram.payload.experiments_enabled && CDH_BEACON_IS_RESULTS(ascii_every_n) && PAYLOAD_is_there_any_experiment_result()) {
	    	cdh_update_beacon_results(&beacon);
            frame_reset_for_reading(&beacon);
		} else
	    if (CDH_BEACON_IS_ASCII(ascii_every_n)) {
	    	cdh_update_beacon_ascii(&beacon);
            frame_reset_for_reading(&beacon);
	    } else {
	    	cdh_update_beacon(&beacon);
            frame_reset_for_reading(&beacon);
            cdh_sample_beacon_tick(&beacon);
	    }

	    lithium_send_data(&beacon);
//	    lithium_set_beacon_data(&beacon);

		rv = POWER_get_battery_voltage(&battery_v);
		if (RV_SUCCESS != rv) battery_v = CDH_BEACON_MIN_BATTERY_VOLTAGE - 0.1f;
		if (battery_v > CDH_BEACON_MAX_BATTERY_VOLTAGE) battery_v = CDH_BEACON_MIN_BATTERY_VOLTAGE - 0.2f;
		if (battery_v < CDH_BEACON_MIN_BATTERY_VOLTAGE) battery_v = CDH_BEACON_MIN_BATTERY_VOLTAGE - 0.3f;

		beaconDelayAccordingToBatteryVoltage_ms = (CDH_BEACON_MAX_BATTERY_VOLTAGE - battery_v) * CDH_BEACON_MS_PER_VOLT + CDH_BEACON_MIN_INTERVAL_MS;
		vTaskDelayUntil(&xLastWakeTime, beaconDelayAccordingToBatteryVoltage_ms / portTICK_RATE_MS);
	}
}
#undef CDH_BEACON_IS_ASCII

#define DISPATCHER_STACKSIZE 2048
static void initialize_cmd_dispatcher_task(const subsystem_t *ss) {
	cdh_subsystem_state_t *ss_state = (cdh_subsystem_state_t *)ss->state;
	portBASE_TYPE rv;

	rv = xTaskCreate(
			&CDH_rx_task,
			(signed char *)"CDH/dispatch_command",
			DISPATCHER_STACKSIZE,
			(void *)ss,
			ss->config->uxPriority,
			&ss_state->rx_task_handle);

	if (pdPASS != rv) {
		SS_FATAL_ERROR(ss);
	}
}

static retval_t configure_radio() {
	int retry;
	retval_t rv;
	uint32_t radio_silence_s;

	vTaskDelay(1000 / portTICK_RATE_MS);

	for (retry=0; retry < 3; retry++) {
		if (nvram.cdh.change_bps_on_every_boot) {
			if (PLATFORM_get_reset_count() % 2) {
				lithium_set_config_bps(&nvram.cdh.default_lithium_configuration, LITHIUM_RF_BAUD_RATE_9600);
			} else {
				lithium_set_config_bps(&nvram.cdh.default_lithium_configuration, LITHIUM_RF_BAUD_RATE_1200);
			}
		}
		rv = lithium_set_configuration(&nvram.cdh.default_lithium_configuration);
		if (RV_SUCCESS == rv) break;
	}

	if (nvram.cdh.one_time_radio_silence_time_s) {
		radio_silence_s = nvram.cdh.one_time_radio_silence_time_s;

		/* Save nvram ASAP. We want radio on next reboot for sure */
		nvram.cdh.one_time_radio_silence_time_s = 0;
		MEMORY_nvram_save(&nvram.cdh.one_time_radio_silence_time_s, sizeof(nvram.cdh.one_time_radio_silence_time_s));

		CDH_delay_beacon(radio_silence_s);
	}

	return rv;
}

static void initialize_radio(const subsystem_t *ss) {
    if (RV_SUCCESS == lithium_initialize(ch_lithium)) {
        configure_radio();
    } else {
        SS_CRITICAL_ERROR(ss);
    }
}

#define ANTENNA_DEPLOY_STACKSIZE 1024
static void initialize_antenna_deploy_task(const subsystem_t *ss) {
	cdh_subsystem_state_t *ss_state = (cdh_subsystem_state_t *)ss->state;
	portBASE_TYPE rv;

	if (!nvram.cdh.antenna_deploy_enabled) {
		log_report(LOG_SS_CDH_ANTENNA, "ANTENNA: deploy inhibited at boot. Reconfigure (nvram) and reset if desired.\n");
		return;
	}

	rv = xTaskCreate(
			&antenna_deploy_task,
			(signed char *)"CDH/Antenna_deploy",
			ANTENNA_DEPLOY_STACKSIZE,
			(void *)ss,
			ss->config->uxPriority,
			&ss_state->antenna_deploy_task_handle);

	if (pdPASS != rv) {
		SS_FATAL_ERROR(ss);
	}
}

#define BEACON_UPDATE_STACKSIZE 1024
static void initialize_beacon_update_task(const subsystem_t *ss) {
	cdh_subsystem_state_t *ss_state = (cdh_subsystem_state_t *)ss->state;
	portBASE_TYPE rv;

	rv = xTaskCreate(
			&beacon_update_task,
			(signed char *)"CDH/Beacon",
			BEACON_UPDATE_STACKSIZE,
			(void *)ss,
			ss->config->uxPriority,
			&ss_state->beacon_update_task_handle);

	if (pdPASS != rv) {
		SS_FATAL_ERROR(ss);
	}
}

static void initialize_channels() {
	retval_t rv;

    rv = channel_open(ch_lithium);
    if (RV_SUCCESS != rv);
}

#define JUST_IN_CASE_BEACON_DATA_MAX_SIZE	100
static void initialize_beacon() {
	char *buf;
	uint32_t uniq;
	int32_t temp = 0x5a7e;
	float gyro;
	int msg_size;
	frame_t radio_beacon = DECLARE_FRAME_SPACE(JUST_IN_CASE_BEACON_DATA_MAX_SIZE);

	(void /*don't care*/)THERMAL_get_temperature(THERMAL_SENSOR_RADIO, &temp);
	(void /*don't care*/)AOCS_get_gyro_magnitude(&gyro);
	uniq  = gyro*100.0f;
	uniq += temp * 0x1000;

	buf = frame_get_data_pointer_nocheck(&radio_beacon, JUST_IN_CASE_BEACON_DATA_MAX_SIZE);
	msg_size = snprintf(buf, JUST_IN_CASE_BEACON_DATA_MAX_SIZE, "Manolito ad astra! Boots:%d Uniq:%08x", PLATFORM_get_reset_count(), uniq);
	frame_advance(&radio_beacon, msg_size);
	frame_reset_for_reading(&radio_beacon);

    lithium_set_beacon_interval(7*60);
    lithium_set_beacon_data(&radio_beacon);
}

static void mode_from_booting_to_initialize(const subsystem_t *ss) {
	initialize_channels();

	frame_pool_initialize();
	initialize_radio(ss);
	initialize_cmd_dispatcher_task(ss);
	initialize_beacon_update_task(ss);
	initialize_antenna_deploy_task(ss);

	cmd_delayed_init();
	cdh_sample_init();
}

#ifdef CMOCKERY_TESTING
void initialize_for_testing(const subsystem_t *ss) {
	static bool already_initialized = false;
	cdh_subsystem_state_t *ss_state = (cdh_subsystem_state_t *)ss->state;
	extern lithium_state_t LITHIUM_STATE; // FIXME :)

	if (already_initialized) return;
	already_initialized = true;

	mode_from_booting_to_initialize(ss);
    vTaskDelete(ss_state->beacon_update_task_handle);
    vTaskDelete(LITHIUM_STATE.umbilical_rx_task_handle);
    vTaskDelete(LITHIUM_STATE.radio_rx_task_handle);
    vTaskDelete(LITHIUM_STATE.radio_tx_task_handle);
}
#endif /* CMOCKERY_TESTING */

static void CDH_main_task(void *pvParameters) {
	satellite_mode_e mode = SM_BOOTING;
	satellite_mode_e prev_mode;

	subsystem_t * ss = (subsystem_t *)pvParameters;

	PLATFORM_ss_is_ready(ss);
	PLATFORM_ss_expect_heartbeats(ss, HEARTBEAT_MAIN_TASK | HEARTBEAT_2nd_TASK | HEARTBEAT_3rd_TASK);
	while (1) {
		if (RV_SUCCESS == PLATFORM_wait_mode_change(ss, &mode, PM_MODE_CHANGE_DETECT_TIME_SLOT_MS / portTICK_RATE_MS)) {
			prev_mode = PLATFORM_previous_satellite_mode();

			FUTURE_HOOK_3(cdh_mode_change, ss, &prev_mode, &mode);
			if (prev_mode == SM_BOOTING && mode == SM_INITIALIZING) {
				mode_from_booting_to_initialize(ss);
			}
			PLATFORM_ss_is_ready(ss);
		} else {
			// No mode change. Tick
			PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);

			FDIR_CDH_check_last_received_command_time();

			switch (mode) {
			case SM_OFF:
			case SM_BOOTING:
				break;
			case SM_INITIALIZING:
			case SM_SURVIVAL:
			case SM_MISSION:
			case SM_LOW_POWER:
				cdh_sample_memory_tick();
				break;
			case SM_COUNT:
				break;
			}
		}
	}
}

/******************************** COMMANDS ****************************/
static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t antenna_deploy_status;
	uint8_t sensors;

	antenna_deploy_status  = nvram.cdh.antenna_deploy_enabled ? 0x80 : 0;
	antenna_deploy_status |= (nvram.cdh.antenna_deploy_tries_counter & 0x7) << 4;

	sensors = 6;
	antenna_get_sensors(&sensors);
	antenna_deploy_status |= sensors;

	frame_put_u32(oframe, nvram.cdh.last_seen_sequence_number);
	frame_put_u8(oframe, antenna_deploy_status);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t value;
	value = nvram.cdh.antenna_deploy_enabled ? 0x80 : 0;
	value += nvram.cdh.last_seen_sequence_number & 0x7f;

	frame_put_bits_by_4(oframe, value, 8);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return RV_SUCCESS;
}

static retval_t cmd_cdh_delay_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint16_t seconds;
	retval_t rv;

	rv = frame_get_u16(iframe, &seconds);
	if (RV_SUCCESS != rv) return rv;

	return CDH_delay_beacon(seconds);
}

static retval_t cmd_cdh_countdown_delay(const subsystem_t *ss, frame_t * iframe, frame_t * oframe, uint32_t seqnum) {
	frame_t *cmd_frame;
	uint16_t count, delay_ms;
	const char msg[] = "Countdown!";
	retval_t rv;

	rv = frame_get_u16(iframe, &count);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u16(iframe, &delay_ms);
	if (RV_SUCCESS != rv) return rv;

	vTaskDelay(delay_ms / portTICK_RATE_MS);

	frame_put_u16(oframe, count);
	frame_put_data(oframe, msg, sizeof(msg));

	count--;
	rv = RV_SUCCESS;
	if (count > 0) {
		rv = CDH_command_new(&cmd_frame, seqnum, SS_CDH, SS_CMD_CDH_COUNTDOWN_DELAY);
		if (RV_SUCCESS == rv) {
			frame_put_u16(cmd_frame, count);
			frame_put_u16(cmd_frame, delay_ms);

			rv = CDH_command_enqueue(cmd_frame);
		}
	}
	return rv;
}

static retval_t cmd_lithium_noop(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return lithium_noop();
}

static retval_t cmd_lithium_reset(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return lithium_reset();
}

static retval_t cmd_radio_reconfigure(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return CDH_radio_reset_and_reconfigure();
}

static retval_t cmd_get_seen_ax25_calls(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	return APRS_add_seen_calls_list_to_frame(oframe);
}

const static ss_command_handler_t subsystem_commands[] = {
	DECLARE_BASIC_COMMANDS("lastSeenSequenceNumber:u32, antennaDeployStatus:u8", "ticksLow:u8"),
	DECLARE_COMMAND(SS_CMD_CDH_DELAY_BEACON, cmd_cdh_delay_beacon, "delayBeacon", "Delay next beacon for some seconds (max 600)", "seconds:u16", ""),
	DECLARE_COMMAND(SS_CMD_CDH_COUNTDOWN_DELAY, cmd_cdh_countdown_delay, "countdown", "Transmit <n> packets, with a delay of <ms> between each of them","times:u16, delay_ms:u16", "count:u16,msg:str"),

	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SET_BEACON_INTERVAL, cmd_radio_set_beacon_interval, "radioSetBeaconInterval", "Sets Radio beacon interval in seconds, not permanent", "seconds:u8", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SET_TX_POWER, cmd_radio_set_tx_power, "radioSetTxPower", "Sets Radio Tx power, not permanent", "txPower:u8", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SET_BEACON_DATA, cmd_radio_set_beacon_data, "radioSetBeacon", "Sets Radio beacon data, not permanent", "data:str", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_GET_CONFIG, cmd_radio_get_config, "radioGetConfig", "Gets Radio configuration", "", "config:str"),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SET_CONFIG, cmd_radio_set_config, "radioSet", "Sets Radio configuration, not permanent", "config:str", "rv:retval_t"),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SEND_FRAME, cmd_radio_send_frame, "txFrame", "Transmits a frame out of the radio", "data:str", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_FLASH_CONFIG, cmd_radio_flash_config, "radioFlashConfig", "Flash radio configuration <md5>. Returns current configuration", "md5:str", "rv:retval_t"),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_GET_TELEMETRY, cmd_radio_get_telemetry, "radioGetTelemetry", "Retrieves Radio's telemetry", "", "telemetry:str"),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SEND_FRAME_AT_BPS, cmd_radio_send_frame_bps, "txFrame", "Transmits a frame out of the radio at the specified BPS, BPS setting is temporary", "bps:u8,data:str", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SET_TX_BPS, cmd_radio_set_tx_bps, "radioSetTx", "Sets Radio's Tx BPS, not permanent", "bps:u8", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_SET_RX_BPS, cmd_radio_set_rx_bps, "radioSetRx", "Sets Radio's rx BPS, not permanent", "bps:u8", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_NOOP, cmd_lithium_noop, "radioNoop", "Send a No-Op command to the radio","", ""),
	DECLARE_COMMAND(SS_CMD_CDH_RADIO_RESET, cmd_lithium_reset, "radioReset", "Reset the radio","", ""),
    DECLARE_COMMAND(SS_CMD_CDH_RADIO_RECONFIGURE, cmd_radio_reconfigure, "radioReconfigure", "Power cycle and reconfigure radio", "", ""),

	DECLARE_COMMAND(SS_CMD_CDH_ANTENNA_DEPLOY_INHIBIT,cmd_antenna_deploy_inhibit, "antennaDeployInhibit", "Inhibits further attempts to deploy the antenna", "flag:u8", ""),

	DECLARE_COMMAND(SS_CMD_CDH_DELAYED_COMMAND, cmd_delayed_add, "delayedCommandAdd", "Adds a command to the delayed commands list. The command will be executed after waiting <seconds>", "waitSeconds:u32", ""),
	DECLARE_COMMAND(SS_CMD_CDH_DELAYED_COMMAND_LIST, cmd_delayed_list, "delayedCommandList",  "Retrieves the list of delayed commands. Only next execution tick count, subsys and cmd", "", ""),
	DECLARE_COMMAND(SS_CMD_CDH_DELAYED_COMMAND_DISCARD, cmd_delayed_discard, "delayedCommandDiscard", "Discards command <n> from the delayed commands list", "index:u8", ""),

	DECLARE_COMMAND(SS_CMD_CDH_SAMPLE_BEACON, cmd_sample_beacon, "samplerBeacon", "Samples <length> bytes of the beacon, starting at <offset>. Do it <count> times every some seconds (limited to beacon update interval)", "offset:u16, length:u16, count:u16, intervalSeconds:u16", ""),
	DECLARE_COMMAND(SS_CMD_CDH_SAMPLE_MEMORY, cmd_sample_memory, "samplerMemory", "Samples <length> bytes of the memory at <address>. Do it <count> times every some seconds (the periodicity is not exact)", "address:u32, length:u16, times:u16, intervalSeconds: u16", ""),
	DECLARE_COMMAND(SS_CMD_CDH_SAMPLE_FLASH_WHEN_DONE, cmd_sample_flash_when_done, "samplerFlashWhenDone", "Enable (1) or disable (0) flashing the sampled data persisten memory when the buffer is full", "enabled:u8", ""),
	DECLARE_COMMAND(SS_CMD_CDH_SAMPLE_RETRIEVE, cmd_sample_retrieve, "samplerRetrieve", "Retrieve sampler's remaining wanted samples count, current buffer size, and the beginning of the sampled data", "", "wanted:u16, position:u16, data:str"),

	DECLARE_COMMAND(SS_CMD_GET_SEEN_AX25_CALLS, cmd_get_seen_ax25_calls, "getSeenCalls", "Retrieve a list of the last 10 AX25 calls received", "", "list:str"),
};

static subsystem_api_t subsystem_api = {
		.main_task = &CDH_main_task,
		.command_execute = &ss_command_execute,
};

extern const ss_tests_t cdh_tests;

static subsystem_config_t subsystem_config = {
		.uxPriority = TASK_PRIORITY_SUBSYSTEM_CDH,
		.usStackDepth = STACK_DEPTH_CDH,
		.id = SS_CDH,
		.name = "CDH",
		.tests = &cdh_tests,
		DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static cdh_subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_CDH = {
		.api    = &subsystem_api,
		.config = &subsystem_config,
		.state  = (subsystem_state_t*)&subsystem_state
};
