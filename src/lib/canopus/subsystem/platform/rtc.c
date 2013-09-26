#include <canopus/subsystem/platform.h>
#include <canopus/nvram.h>

static uint64_t delta_to_cpu_uptime_ms;

uint64_t rtc_get_current_time() {
	return delta_to_cpu_uptime_ms + PLATFORM_get_cpu_uptime_ms();
}

void rtc_set_current_time(uint64_t current_time_ms) {
	delta_to_cpu_uptime_ms = current_time_ms - PLATFORM_get_cpu_uptime_ms();
//	nvram.platform.last_known_time_utc_ms = current_time_ms;
//	MEMORY_nvram_save(&nvram.platform.last_known_time_utc_ms, sizeof(nvram.platform.last_known_time_utc_ms));
}
