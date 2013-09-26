#include <stdlib.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#include <canopus/cpu.h>

#define FREERTOS_STAT_BUFSIZE_PER_TASK 40
#define CANOPUS_CMOCKERY_ESTIMATED_TASKS_COUNT 42

void
cpu_reset0()
{
    fflush(NULL); /* flushes all open output streams */
    exit(1);
}

static inline signed char
isRunTimeStatsBufferInteresting(signed char *freertos_filled_buf)
{
    return freertos_filled_buf[2] != '\0';
}

void
cpu_reset2(const char *reason, int errcode)
{
#if ( configGENERATE_RUN_TIME_STATS == 1 )
    signed char buf[CANOPUS_CMOCKERY_ESTIMATED_TASKS_COUNT * FREERTOS_STAT_BUFSIZE_PER_TASK];

    vTaskGetRunTimeStats(buf);
    if (isRunTimeStatsBufferInteresting(buf)) {
        puts((const char *)buf);
    }
#endif /* configGENERATE_RUN_TIME_STATS == 1 */
    puts(reason);

    exit(errcode);
}
