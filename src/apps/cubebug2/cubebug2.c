#include <FreeRTOS.h>
#include <task.h>

#include <canopus/board.h> /* board_init_scheduler_not_running() */
#include <canopus/logging.h>
#include <canopus/subsystem/platform.h>

#include <canopus/drivers/flash/ramcopy.h> /* harakiri */

static void
harakiri(uint32_t index)
{
#ifdef HARAKIRI_ENABLED
    extern uint16_t __stage_0_config__;
    const uint16_t *stage0conf_addr = (uint16_t *)&__stage_0_config__;
    flash_err_t rv;

    FUTURE_HOOK_1(harakiri_0, &index);

    if (index % sizeof(uint16_t) != 0) return;
    if (index > 0x4000) return;

    index >>= 1;

    // XXX must not be run XiP!!!
    rv = _flash_init(); /* call that first to close flash if it's opened */

    if (0 != stage0conf_addr[index-1]) return;
    if (0 == stage0conf_addr[index])   return;

    if (FLASH_ERR_OK != rv) {
#       ifdef LOG_REPORT_BEFORE_BOARD_INIT_ALLOWED
        log_report(LOG_GLOBAL, "harakiri FAILING?!\n");
#       endif
    }

    rv = _flash_write((void *)&stage0conf_addr[index], "\x00\x00", 2);
#   ifdef LOG_REPORT_BEFORE_BOARD_INIT_ALLOWED
    log_report_fmt(LOG_GLOBAL, "harakiri @%08x: %s\n", &stage0conf_addr[index], rv == FLASH_ERR_OK ? "SUCCESS" : "FAILURE");
#   endif

#endif /* HARAKIRI_ENABLED */
}

int app_main( void )
{
	board_init_scheduler_not_running();

    harakiri(0x08);

    xTaskCreate(
			SUBSYSTEM_PLATFORM.api->main_task,
			(signed char *) SUBSYSTEM_PLATFORM.config->name,
			SUBSYSTEM_PLATFORM.config->usStackDepth,
			(void*)&SUBSYSTEM_PLATFORM,
			SUBSYSTEM_PLATFORM.config->uxPriority, NULL);

	vTaskStartScheduler();

	// TODO

	return 69;
}
