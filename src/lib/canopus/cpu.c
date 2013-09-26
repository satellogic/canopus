#include <canopus/types.h>
#include <canopus/cpu.h>
#include <canopus/drivers/console_lowlevel.h>

#include <FreeRTOS.h>
#include <task.h>

void
cpu_reset(const char *reason)
{
    FUTURE_HOOK_1(cpu_reset_0, reason);
    portDISABLE_INTERRUPTS();
    FUTURE_HOOK_1(cpu_reset_1, reason);

    if (reason != NULL) {
        lowlevel_console_putnewline();
        lowlevel_console_putstring("CPU RESET: ");
        lowlevel_console_putstring(reason);
        lowlevel_console_putnewline();
    }

    cpu_reset0();
}
