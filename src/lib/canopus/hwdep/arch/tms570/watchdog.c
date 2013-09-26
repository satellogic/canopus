#include <FreeRTOS.h>
#include <task.h>

#include <canopus/types.h>
#include <canopus/cpu.h>
#include <canopus/watchdog.h>
#include <canopus/logging.h>

#define WATCHDOG_TASK_STACKSIZE 1000

static struct {
    bool restart_watchdog_on_next_interrupt;
    bool a_new_period_has_started;
    uint32_t period_s;

    xTaskHandle task_handle;
} wctx = { false, false, 0, NULL };

void watchdog_force_cpu_reset() {
	portENTER_CRITICAL();

    FUTURE_HOOK_0(watchdog_force_cpu_reset);

    // TODO

    cpu_reset("WATCHDOG RESET");

    while (1) { /* loop until hardware is reset */};
}

void watchdog_disable() {
	if (NULL != wctx.task_handle) vTaskSuspend(wctx.task_handle);
	// TODO?
}

void watchdog_restart_counter() {
	wctx.restart_watchdog_on_next_interrupt = true;
	// TODO
}

bool watchdog_has_started_a_new_period() {
	bool answer;
	answer = wctx.a_new_period_has_started;
	if (answer)	wctx.a_new_period_has_started = false;
	return answer;
}

extern void commhub_kick();

static void watchdog_task(void *arg_ignored) {

    /* since we start with highest priority, we start suspended, watchdog_enable() will resume us */
    vTaskSuspend(NULL);

	while (1) {
		wctx.a_new_period_has_started = true;
		vTaskDelay(wctx.period_s * 1000 / portTICK_RATE_MS);
		commhub_kick();
		log_report(LOG_WDG, "WATCHDOG tick\n");
		if (!wctx.restart_watchdog_on_next_interrupt) {
			log_report(LOG_ALL, "WATCHDOG Timed Out!!!\n");
			//watchdog_force_cpu_reset(); // TODO
		}
		wctx.restart_watchdog_on_next_interrupt = false;
	}
}

retval_t watchdog_enable(uint32_t seconds) {
    wctx.period_s = seconds;
    if ((NULL == wctx.task_handle) && (pdPASS != xTaskCreate(watchdog_task, (signed char*)"Watchdog", WATCHDOG_TASK_STACKSIZE, NULL, WATCHDOG_TASK_PRIORITY, &wctx.task_handle))) {
        return RV_ERROR;
    } else {
        watchdog_disable();
        wctx.restart_watchdog_on_next_interrupt = false;
    }

    vTaskResume(wctx.task_handle);
	log_report_fmt(LOG_ALL, "WATCHDOG Enabled: %d seconds interval\n", wctx.period_s);

	return RV_SUCCESS;
}
