#include <canopus/drivers/console_lowlevel.h>
#include <canopus/cpu.h>
#include <ctype.h>

#include <FreeRTOS.h>
#include <task.h>

#if( configCHECK_FOR_STACK_OVERFLOW > 0 )

/* if the task stack is partly destroyed, pcTaskName may be smashed, so may
 * point to some unmapped memory, leading to a DATA ABORT.
 * this may be safer if we run using MPU */
static inline void
display_taskname(signed portCHAR *pcTaskName)
{
#ifdef FREERTOS_STACKOVERFLOW_DISPLAY_TASKNAME_SAFE
    int i;

    for (i = 0; i < configMAX_TASK_NAME_LEN; i++) {
        signed portCHAR c = pcTaskName[i];

        if ('\0' == c) break;
        if (!isascii(c)) return;
    }
    lowlevel_console_putstring("in task '");
    lowlevel_console_putstring((const char *)pcTaskName);
    lowlevel_console_putstring("' ");
#endif
}

void
vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName)
{
    lowlevel_console_putnewline();
    lowlevel_console_putstring("STACK OVERFLOW");
    lowlevel_console_putnewline();
    display_taskname(pcTaskName);
    lowlevel_console_putstring(" at 0x");
    lowlevel_console_puthex32((uint32_t)pxTask); /* may be inconsistant */
    lowlevel_console_putnewline();

    /* no MPU -> reset */
    cpu_reset("STACK OVERFLOW");
}

#endif /* configCHECK_FOR_STACK_OVERFLOW > 0 */

#if( configUSE_MALLOC_FAILED_HOOK == 1 )

void
vApplicationMallocFailedHook( void )
{
    lowlevel_console_putnewline();
    lowlevel_console_putstring("MALLOC FAILED");
    lowlevel_console_putnewline();
    cpu_reset("MALLOC FAILED"); /* FIXME implement policies */
}

#endif /* configUSE_MALLOC_FAILED_HOOK == 1 */
