#ifndef _CANOPUS_SUBSYSTEM_PLATFORM_H_
#define _CANOPUS_SUBSYSTEM_PLATFORM_H_

#include <canopus/subsystem/subsystem.h>

extern subsystem_t SUBSYSTEM_PLATFORM;

#define PM_MODE_CHANGE_WAIT_TIME_SLOT_MS	100
#define PM_MODE_CHANGE_DETECT_TIME_SLOT_MS	2500
#define PM_SS_BOOT_TIMEOUT_MS				1000
#define PM_SS_MODE_CHANGE_TIMEOUT_MS		15000
#define SYSTEM_WATCHDOG_INTERVAL_s			(4*60)

typedef struct nvram_platform_t {
	uint16_t reset_count;
	void *last_boot_reason;
} nvram_platform_t;

satellite_mode_e PLATFORM_current_satellite_mode(void);
satellite_mode_e PLATFORM_previous_satellite_mode(void);
void PLATFORM_request_mode_change(satellite_mode_e new_mode);
void PLATFORM_ss_is_ready(subsystem_t *ss);
void PLATFORM_ss_is_alive(subsystem_t *ss, uint8_t heartbeat_bit);
void PLATFORM_save_boot_reason(const char *reason);
void PLATFORM_save_boot_reason_unexpected();
void PLATFORM_ss_expect_heartbeats(subsystem_t *ss, uint8_t heartbeat_bits);
retval_t PLATFORM_wait_mode_change(subsystem_t *ss, satellite_mode_e *mode, portTickType ticksTimeout);
subsystem_t *PLATFORM_get_subsystem(ss_id_e ss_id);
__attribute__((noreturn)) void PLATFORM_safe_reset(const char *reason);
uint16_t PLATFORM_get_reset_count();
uint32_t PLATFORM_get_cpu_uptime_ms();
uint32_t PLATFORM_get_cpu_uptime_s();
#endif
