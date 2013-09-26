/* platform manager subsystem */
#include <canopus/assert.h>
#include <canopus/cpu.h>
#include <canopus/watchdog.h>
#include <canopus/version.h>
#include <canopus/logging.h>
#include <canopus/board.h> /* board_init() */
#include <canopus/drivers/flash.h> /* flash_init */
#include <canopus/drivers/leds.h>
#include <canopus/board/channels.h>
#include <canopus/board/adc.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/subsystem/platform.h>
#include <canopus/subsystem/mm.h>
#include <canopus/nvram.h>
#include <canopus/drivers/commhub_1500.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "rtc.h"

static satellite_mode_e g_mode, g_previous_mode, g_next_mode;
static void *saved_last_boot_reason;

#define MODE_NAME(MODE)		[SM_##MODE]     = #MODE
static const char *smode[] = {
	MODE_NAME(OFF),
	MODE_NAME(BOOTING),
	MODE_NAME(INITIALIZING),
	MODE_NAME(SURVIVAL),
	MODE_NAME(MISSION),
	MODE_NAME(LOW_POWER)
};

satellite_mode_e PLATFORM_current_satellite_mode() {
    return g_mode;
}

satellite_mode_e PLATFORM_previous_satellite_mode() {
    return g_previous_mode;
}

void PLATFORM_request_mode_change(satellite_mode_e new_mode) {
    log_report_fmt(LOG_SS_PLATFORM, "MODE change request: %s -> %s\r\n", smode[g_mode], smode[new_mode]);
	g_next_mode = new_mode;
}

void PLATFORM_ss_is_ready(subsystem_t *ss) {
	ss->state->status = SS_ST_READY;
}

void PLATFORM_ss_expect_heartbeats(subsystem_t *ss, uint8_t heartbeat_bits) {
	ss->state->expected_heartbeats = heartbeat_bits;
}

void PLATFORM_ss_is_alive(subsystem_t *ss, uint8_t heartbeat_bit) {
	ss->state->current_heartbeats |= heartbeat_bit;
}

void PLATFORM_save_boot_reason(const char *reason) {
	if (nvram.platform.last_boot_reason == reason) return;
	nvram.platform.last_boot_reason = (void*)reason;
	MEMORY_nvram_save(&nvram.platform.last_boot_reason, sizeof(nvram.platform.last_boot_reason));
}

void PLATFORM_save_boot_reason_unexpected() {
	PLATFORM_save_boot_reason("Unexpected reboot");
}

uint16_t PLATFORM_get_reset_count() {
	return nvram.platform.reset_count;
}

uint32_t  PLATFORM_get_cpu_uptime_ms() {
	return xTaskGetTickCount() / portTICK_RATE_MS;
}

uint32_t PLATFORM_get_cpu_uptime_s() {
	return PLATFORM_get_cpu_uptime_ms() / 1000;
}

subsystem_t *PLATFORM_get_subsystem(ss_id_e ss_id) {
	if (ss_id < ARRAY_COUNT(subsystems)) {
		return subsystems[ss_id];
	} else {
		return NULL;
	}
}

__attribute__((noreturn))
void PLATFORM_safe_reset(const char *reason) {
    /* TODO: MM_save_nvram(); */
	PLATFORM_save_boot_reason((void*)reason);
	POWER_reset();
	vTaskDelay(1000 / portTICK_RATE_MS);
    cpu_reset(reason);
}

/* PM_wait_mode_change() is used by other subsystems to poll/block for a mode change request from PM */
retval_t PLATFORM_wait_mode_change(const subsystem_t *ss, satellite_mode_e *mode, portTickType ticksTimeout) {
	signed portBASE_TYPE rv;

	rv = xSemaphoreTake(ss->state->semphr, ticksTimeout);
	if (pdTRUE == rv) {
		*mode = PLATFORM_current_satellite_mode();
		return RV_SUCCESS;
	}
	return RV_TIMEOUT;
}

#define SEMAPHORE_TIMEOUT_POLLING   0
#define SEMAPHORE_TIMEOUT_BLOCKING  portMAX_DELAY

static retval_t subsystemTaskCreate(subsystem_t* ss) {
    subsystem_config_t* ss_config = ss->config;
    subsystem_state_t* ss_state = ss->state;
    portBASE_TYPE rv;

    log_report_fmt(LOG_SS_PLATFORM, "Creating subsystem task: %s (stack size: %d bytes)\r\n", ss_config->name, ss_config->usStackDepth * sizeof( portSTACK_TYPE ));
	ss_state->status = SS_ST_BOOTING;
	PLATFORM_ss_expect_heartbeats(ss, HEARTBEAT_MAIN_TASK);
    vSemaphoreCreateBinary(ss_state->semphr);
    assert(NULL != ss_state->semphr);
    rv = xSemaphoreTake(ss_state->semphr, SEMAPHORE_TIMEOUT_POLLING);
    assert(pdTRUE == rv);

    assert(ss_config->uxPriority < configMAX_PRIORITIES);
    assert(ss_config->uxPriority < WATCHDOG_TASK_PRIORITY);
    rv = xTaskCreate(
    		ss->api->main_task,
    		(signed char *)ss_config->name,
            configMINIMAL_STACK_SIZE + ss_config->usStackDepth,
    		(void *)ss,
    		ss_config->uxPriority,
    		&ss_state->task_handle);
    assert(pdPASS == rv);

    return rv == pdPASS ? RV_SUCCESS:RV_ERROR;
}

static __attribute__((noreturn))
void hang(void) {
	while (1) {vTaskDelay(100);}
}

static inline
bool IS_SS_VALID(ss_id_e i) {
    return board_enabled_subsystem_id(i) && IS_PTR_VALID(subsystems[i]);
}

static void report_ss_states_and_hang(bool do_hang) {
	ss_id_e i;
    bool ready, all_ready;
    all_ready = true;
    for (i=0; i < ARRAY_COUNT(subsystems); i++) {
        if (!IS_SS_VALID(i)) continue;
    	ready = subsystems[i]->state->status == SS_ST_READY;
    	all_ready &= ready;
		if (!ready) {
			log_report_fmt(LOG_SS_PLATFORM, "Subsystem %s not ready\r\n", subsystems[i]->config->name);
		}
    }
    if (do_hang && !all_ready) hang();
}

static void start_ss_tasks() {
	ss_id_e i;

	for (i=0; i < ARRAY_COUNT(subsystems); i++) {
		if (i == SS_PLATFORM) continue;
        if (!IS_SS_VALID(i)) continue;

		(void)subsystemTaskCreate(subsystems[i]);
	}
}

static bool are_all_ss_ready() {
	ss_id_e i;

    for (i=0; i < ARRAY_COUNT(subsystems); i++) {
        if (!IS_SS_VALID(i)) continue;
		if (SS_ST_READY != subsystems[i]->state->status) {
			return false;
		}
    }
    return true;
}

static bool are_all_vital_ready() {
	return
		SS_ST_READY == subsystems[SS_MEMORY]->state->status &&
		SS_ST_READY == subsystems[SS_POWER]->state->status &&
		SS_ST_READY == subsystems[SS_CDH]->state->status;
}

static bool wait_all_ss_ready(portTickType xTicksTimeout) {
	bool ready;

	xTicksTimeout += xTaskGetTickCount();
    do {
        vTaskDelay(PM_MODE_CHANGE_WAIT_TIME_SLOT_MS / portTICK_RATE_MS);
        ready = are_all_ss_ready();
    } while ((!ready) && (xTicksTimeout > xTaskGetTickCount()));
    return ready;
}

static retval_t mode_change_synchronous(satellite_mode_e new_mode, portTickType perSSTimeout_ticks) {
	// PM Has the highest priority, so xSemaphoreGive() will not force a context switch
	ss_id_e i;
    bool ready;
    retval_t answer;
    portTickType timeout;
    subsystem_state_t *ss_state;
//    portBASE_TYPE rv;

    answer = RV_SUCCESS;

    g_previous_mode = g_mode;
    g_mode = new_mode;

    for (i=0; i < ARRAY_COUNT(subsystems); i++) {
		if (i == SS_PLATFORM) continue;
        if (!IS_SS_VALID(i)) continue;

		ss_state = subsystems[i]->state;
		ss_state->status = SS_ST_MODE_CHANGE_PENDING;

        log_report_fmt(LOG_SS_PLATFORM, "Changing mode for %s (%s -> %s).\r\n", subsystems[i]->config->name, smode[g_previous_mode], smode[g_mode]);
		timeout = perSSTimeout_ticks + xTaskGetTickCount();
        /* rv = */ xSemaphoreGive(ss_state->semphr);
//        assert(pdTRUE == rv);

        do {
        	ready = (SS_ST_READY == ss_state->status);
            if (!ready) {
            	vTaskDelay(PM_MODE_CHANGE_WAIT_TIME_SLOT_MS / portTICK_RATE_MS);
            }
        } while ((!ready) && (timeout > xTaskGetTickCount()));
        log_report_fmt(LOG_SS_PLATFORM, "%s is %s.\r\n", subsystems[i]->config->name, ready?"ready":"not ready");
        if (!ready) answer = RV_TIMEOUT;
    }
    report_ss_states_and_hang(false);
    return answer;
}

static void check_and_update_heartbeat_watchdog() {
	ss_id_e i;
	bool all_alive;
	subsystem_t *ss;

	all_alive = true;

	if (watchdog_has_started_a_new_period()) {
//		log_report(LOG_SS_PLATFORM, "PLATFORM: All subsystem's heartbeats cleared\n");
	    for (i=0; i < ARRAY_COUNT(subsystems); i++) {
	        if (IS_SS_VALID(i)) {
                subsystems[i]->state->current_heartbeats = 0;
            }
	    }
	} else {
		for (i=0; i < ARRAY_COUNT(subsystems); i++) {
			if (!IS_SS_VALID(i)) continue;
			ss = subsystems[i];

			if ((ss->state->current_heartbeats & ss->state->expected_heartbeats) != ss->state->expected_heartbeats) {
				all_alive = false;
				PLATFORM_save_boot_reason(ss->config->name);
//				log_report_fmt(LOG_SS_PLATFORM, "PLARFORM: Subsystem %s missed a heartbeat (%02x != %02x)\n",
//						ss->config->name,
//						ss->state->current_heartbeats,
//						ss->state->expected_heartbeats);
			}
		}

		if (all_alive) {
//			log_report(LOG_SS_PLATFORM, "PLATFORM: All subsystems are alive!\n");
			watchdog_restart_counter();
			POWER_watchdog_restart_counter();
			led_toggle(1); /* triggers the Alive LED */
			PLATFORM_save_boot_reason_unexpected();
		}
	}
}

static inline
void display_boot_banner() {
#ifdef CONSOLE_POLLING_BUG_THAT_BRICK_UMBILICAL_UART_FIXED
    lowlevel_console_putstring("Booting Manolito (CubeBug-2)\n Version ");
    lowlevel_console_putstring(build_git_sha);
    lowlevel_console_putstring("\n ");
    lowlevel_console_putstring(build_git_time);
    lowlevel_console_putnewline();
#else
    log_report_fmt(LOG_SS_PLATFORM, "Booting Manolito (CubeBug-2)\r\n Version %s @0x%08x\r\n %s\r\n", build_git_sha, MEMORY_fw_start_address(), build_git_time); // XXX lowlevel
#endif
}

static void initialize_nvram() {
    /* belong to MM but isn't MM-dependent, can be called before MM task initialized
     * FIXME would be better to call _after_ flash_init()... (in case of flash locked)
     */
    flash_init();
    MEMORY_nvram_reload();
}

static void increment_reboot_counter() {
	nvram.platform.reset_count++;
	MEMORY_nvram_save(&nvram.platform.reset_count, sizeof(nvram.platform.reset_count));
}

static void initialize_devices() {
    watchdog_enable(SYSTEM_WATCHDOG_INTERVAL_s);

    (void /* what to do if it fails? */)channel_open(ch_fpga_ctrl);

// Is it safer to do it? commhub_connectUarts(COMMHUB_UART_ADDR_TMS_0_UART_1, COMMHUB_UART_ADDR_LITHIUM);
//    commhub_connectUarts(COMMHUB_UART_ADDR_TMS_0_UART_0, COMMHUB_UART_ADDR_UMB);
//    commhub_disconnectUartRx(COMMHUB_UART_ADDR_CONDOR);
//    commhub_disconnectUartRx(COMMHUB_UART_ADDR_SVIP_0);
//    commhub_disconnectUartRx(COMMHUB_UART_ADDR_SVIP_1);
//    commhub_disconnectUartRx(COMMHUB_UART_ADDR_OVERO_UART_1);
}

static void PLATFORM_main_task(void* pvParameters) {
	subsystem_t *ss = pvParameters;
    g_previous_mode = SM_OFF;
    g_mode = g_next_mode = SM_BOOTING;
    retval_t rv;

    initialize_nvram();
    increment_reboot_counter();
    saved_last_boot_reason = nvram.platform.last_boot_reason;
    PLATFORM_save_boot_reason("Unexpected reboot");

	rv = board_init_scheduler_running();
    FUTURE_HOOK_1(PM_board_init_failure, &rv);
	if (rv != RV_SUCCESS) {
        /* XXX save in subsystem status, report in beacons */
	}

    display_boot_banner();

    initialize_devices();

	PLATFORM_ss_is_ready(ss);
	PLATFORM_ss_expect_heartbeats(ss, HEARTBEAT_MAIN_TASK);

	start_ss_tasks();
    if (!wait_all_ss_ready(PM_SS_BOOT_TIMEOUT_MS / portTICK_RATE_MS)) {
    	report_ss_states_and_hang(true);
    }

    while (1) {
		PLATFORM_ss_is_alive(ss, HEARTBEAT_MAIN_TASK);

    	if (g_mode != g_next_mode) {
			FUTURE_HOOK_3(platform_mode_change, ss, &g_mode, &g_next_mode);
    		mode_change_synchronous(g_next_mode, PM_SS_MODE_CHANGE_TIMEOUT_MS);
			PLATFORM_ss_is_ready(ss);
    	}

    	switch (PLATFORM_current_satellite_mode()) {
			case SM_BOOTING:
				PLATFORM_request_mode_change(SM_INITIALIZING);
		        break;
			case SM_INITIALIZING:
				if (are_all_vital_ready()) {
					PLATFORM_request_mode_change(SM_SURVIVAL);
				} else {
					/* Do nothing. Ground Control will decide what to do */
				}
			default:
				break;
    	}

    	check_and_update_heartbeat_watchdog();

    	vTaskDelay(PM_MODE_CHANGE_DETECT_TIME_SLOT_MS / portTICK_RATE_MS);
    }
}

/**************************** COMMANDS ************************/

static retval_t cmd_get_telemetry_beacon(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_put_u32(oframe, PLATFORM_get_cpu_uptime_s());
	frame_put_u24(oframe, nvram.platform.reset_count);
	frame_put_u8(oframe, PLATFORM_current_satellite_mode());
	frame_put_u32(oframe, (uint32_t)saved_last_boot_reason);
    FUTURE_HOOK_2(pm_cmd_get_telemetry_beacon, iframe, oframe);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_short(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_put_bits_by_4(oframe, xTaskGetTickCount(), 6);
	frame_put_bits_by_4(oframe, PLATFORM_current_satellite_mode()-2, 2);
    FUTURE_HOOK_2(pm_cmd_get_telemetry_beacon_short, iframe, oframe);
	return RV_SUCCESS;
}

static retval_t cmd_get_telemetry_beacon_ascii(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	char buf[20];
	uint32_t uptime_s;
	uint8_t h,m,s;
	int n;

	uptime_s = PLATFORM_get_cpu_uptime_s();
	h = uptime_s / 3600;
	m = uptime_s % 3600 / 60;
	s = uptime_s % 60;

	n = snprintf(buf, sizeof(buf), "Upt: %02d:%02d:%02d ", h, m, s);
	if (n < 0) {
		return RV_ERROR;
	}
	return frame_put_data(oframe, buf, n);
}

static retval_t cmd_set_mode(const  subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t new_mode;
	retval_t rv;

	rv = frame_get_u8(iframe, &new_mode);
	if (RV_SUCCESS == rv) {
        switch (new_mode) {
        case SM_SURVIVAL:
        case SM_MISSION:
        case SM_LOW_POWER:
            PLATFORM_request_mode_change(new_mode);
            break;
        default:
        	frame_put_u8(oframe, RV_ILLEGAL);
            return RV_ILLEGAL;
        }
    }

	return RV_SUCCESS;
}

__attribute__((noreturn)) static
void cmd_soft_reset_system(const  subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    FUTURE_HOOK_2(pm_cmd_soft_reset_system, iframe, oframe);
	PLATFORM_safe_reset("Ground command");
}

#define SPACE_FOR_NUL_CHARACTER 1
static retval_t cmd_get_version(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    return frame_put_data(oframe, build_git_sha, sizeof(build_git_sha)-SPACE_FOR_NUL_CHARACTER);
}
#undef SPACE_FOR_NUL_CHARACTER

static retval_t cmd_watchdog_enable(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint32_t key;
	uint16_t seconds;
    retval_t rv;

	if (RV_SUCCESS != frame_get_u32(iframe, &key)) return RV_ILLEGAL;
	if (RV_SUCCESS != frame_get_u16(iframe, &seconds)) return RV_ILLEGAL;

    FUTURE_HOOK_4(pm_cmd_watchdog_enable, &key, &seconds, iframe, oframe);

    if (0xCACACECA != key) return RV_ILLEGAL;
    log_report_fmt(LOG_SS_PLATFORM, "PLATFORM: setting watchdog to %d\r\n", seconds);

    rv = watchdog_enable(seconds);
    if (RV_SUCCESS == rv) {
        log_report_fmt(LOG_SS_PLATFORM, "PLATFORM: set watchdog to %d!\r\n", seconds);
    } else {
        (void)frame_put_u32(oframe, rv);
    }

    return rv;
}

static retval_t cmd_watchdog_disable(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	uint32_t key;

	if (RV_SUCCESS != frame_get_u32(iframe, &key)) return RV_ILLEGAL;
    FUTURE_HOOK_3(pm_cmd_watchdog_disable, &key, iframe, oframe);

    if (0xCACACECA != key) return RV_ILLEGAL;

    watchdog_disable();
    return RV_SUCCESS;
}

static retval_t cmd_set_debugmask(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
    logmask_t old_mask, new_mask;

    if (RV_SUCCESS == frame_get_u64(iframe, &new_mask)) {
        old_mask = log_setmask(new_mask, true);
        if (old_mask != new_mask) {
            frame_put_u64(oframe, old_mask);
        }
    }

    return RV_SUCCESS;
}

static retval_t cmd_dump_runtime_stats(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
#if ( configGENERATE_RUN_TIME_STATS == 1 )
#define FREERTOS_STAT_BUFSIZE_PER_TASK 40
#define CANOPUS_CMOCKERY_ESTIMATED_TASKS_COUNT 42
    signed char buf[CANOPUS_CMOCKERY_ESTIMATED_TASKS_COUNT * FREERTOS_STAT_BUFSIZE_PER_TASK];

    vTaskGetRunTimeStats(buf);
    log_report(LOG_SS_PLATFORM, (const char *)buf);
    FUTURE_HOOK_4(cmd_dump_runtime_stats, buf, sizeof(buf), iframe, oframe);

    return RV_SUCCESS;
#else
    return RV_NOTIMPLEMENTED;
#endif
}

static retval_t cmd_uart_connect(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	uint8_t deviceA, deviceB;
	retval_t rv;

	frame_get_u8(iframe, &deviceA);
	rv = frame_get_u8(iframe, &deviceB);
	SUCCESS_OR_RETURN(rv);

	return commhub_connectUarts(deviceA, deviceB);
}

static retval_t cmd_uart_disconnect(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	uint8_t device;
	retval_t rv;

	rv = frame_get_u8(iframe, &device);
	SUCCESS_OR_RETURN(rv);

	return commhub_disconnectUartRx(device);
}

static retval_t cmd_adc_vref_stats(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	return 	adc_get_25vref_stats(oframe);
}

static retval_t cmd_adc_measure_panels(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	int i;
	float samples[ADC_CHANNELS];
	adc_get_samples(ADC_BANK_SOLAR, samples);
	for (i=3; i < ADC_CHANNELS; ++i){
		frame_put_u16(oframe, (uint16_t)(samples[i]*1000));
	}
	return RV_SUCCESS;
}

static retval_t cmd_rtc_get_time(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	return frame_put_u64(oframe, rtc_get_current_time());
}

static retval_t cmd_rtc_set_time(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	uint64_t current_time_ms;
	retval_t rv;

	rv = frame_get_u64(iframe, &current_time_ms);
	SUCCESS_OR_RETURN(rv);

	rtc_set_current_time(current_time_ms);
	return frame_put_u64(oframe, rtc_get_current_time());
}

static retval_t cmd_get_fpga_ticks(const subsystem_t *self, frame_t *iframe, frame_t *oframe) {
	uint64_t ticks;
	retval_t rv;

	rv = commhub_readCounter(&ticks);
	SUCCESS_OR_RETURN(rv);

	return frame_put_u64(oframe, ticks);
}

const static ss_command_handler_t subsystem_commands[] = {
    DECLARE_BASIC_COMMANDS("uptime_s:u32, resetCount:u24, currentMode:u8, lastBootReason:u32", ""),
    DECLARE_COMMAND(SS_CMD_PM_SET_MODE, cmd_set_mode, "set", "Force a satellite mode change to the new mode", "mode:u8", ""),
    DECLARE_COMMAND(SS_CMD_PM_SOFT_RESET_SYSTEM, cmd_soft_reset_system, "reset", "reset: Force CPU reset", "", ""),
    DECLARE_COMMAND(SS_CMD_PM_GET_VERSION, cmd_get_version, "version", "Returns ascii rep of the git commit (40 bytes)", "", "version:str"),
    DECLARE_COMMAND(SS_CMD_PM_WATCHDOG_ENABLE, cmd_watchdog_enable, "watchdogEnable", "Enable system watchdog <key = 0xcacaceca>","key:u32,seconds:u16", ""),
    DECLARE_COMMAND(SS_CMD_PM_WATCHDOG_DISABLE, cmd_watchdog_disable, "watchdogDisable", "Disable system watchdog <key = 0xcacaceca>","key:u32", ""),
    DECLARE_COMMAND(SS_CMD_PM_SET_DEBUGLEVEL, cmd_set_debugmask, "setDebug", "Set console debug mask", "mask:u64", "mask:u64"),
    DECLARE_COMMAND(SS_CMD_PM_DUMP_RUNTIME_STATS, cmd_dump_runtime_stats, "dumpRuntimeStats", "Dump FreeRTOS Runtime stats", "", ""),
    DECLARE_COMMAND(SS_CMD_PM_UART_CONNECT, cmd_uart_connect, "connect", "Connect two uarts via FPGA", "uart:u8,with:u8", ""),
    DECLARE_COMMAND(SS_CMD_PM_UART_DISCONNECT, cmd_uart_disconnect, "disconnect", "Disconnect an UART via FPGA", "uart:u8", ""),
    DECLARE_COMMAND(SS_CMD_PM_ADC_VREF_STATS, cmd_adc_vref_stats, "adcVrefStats", "Get ADC 2.5V vref statistics", "", "last:u16,smallest_seen:u16,biggest_seen:u16"),
    DECLARE_COMMAND(SS_CMD_PM_ADC_MEASURE_PANELS, cmd_adc_measure_panels, "adcMeasurePanels", "Measure solar panels with ADC in mV", "", "sol1P:u16,sol1N:u16,temp1P:u16,temp1N:u16,sol2P:u16,sol2N:u16,temp2P:u16,temp2N:u16,sol3P:u16,sol3N:u16,temp3P:u16,temp3N:u16"),
    DECLARE_COMMAND(SS_CMD_PM_GET_ON_BOARD_TIME, cmd_rtc_get_time, "getTimeMS", "Get onboard time", "", "timeMs:u64:[Time fromSeconds: timeMs/1000]"),
    DECLARE_COMMAND(SS_CMD_PM_SET_ON_BOARD_TIME, cmd_rtc_set_time, "set", "Set onboard time", "timeMs:u64", "setTimeMs:u64:[Time fromSeconds: setTimeMs/1000]"),
    DECLARE_COMMAND(SS_CMD_PM_GET_FPGA_TICKS, cmd_get_fpga_ticks, "getFpgaTicks", "Get Tick counter from FPGA", "", "ticks:u64"),
};

static subsystem_api_t subsystem_api = {
    .main_task = &PLATFORM_main_task,
    .command_execute = &ss_command_execute,
};

extern const ss_tests_t platform_tests;

static subsystem_config_t subsystem_config = {
    .uxPriority = TASK_PRIORITY_SUBSYSTEM_PLATFORM,
    .usStackDepth = STACK_DEPTH_PLATFORM,
    .id = SS_PLATFORM,
    .name = "PLATFORM",
    .tests = &platform_tests,
    DECLARE_COMMAND_HANDLERS(subsystem_commands),
};

static subsystem_state_t subsystem_state;

subsystem_t SUBSYSTEM_PLATFORM = {
    .api    = &subsystem_api,
    .config = &subsystem_config,
    .state  = &subsystem_state
};
