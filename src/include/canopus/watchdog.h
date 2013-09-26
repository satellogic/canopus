/* FIXME may be moved into drivers/ */
#ifndef __CANOPUS_WATCHDOG_H__
#define __CANOPUS_WATCHDOG_H__
#include <FreeRTOS.h>
#include <canopus/types.h>

#define WATCHDOG_TASK_PRIORITY (configMAX_PRIORITIES - 1) // FIXME must be > TASK_PRIORITY_SUBSYSTEM_PLATFORM!!!

void __attribute__((noreturn)) watchdog_force_cpu_reset(void);

retval_t watchdog_enable(uint32_t seconds);
void watchdog_disable(void);
#define watchdog_kick() watchdog_restart_counter()
void watchdog_restart_counter(void);
bool watchdog_has_started_a_new_period(void);

#endif /* __CANOPUS_WATCHDOG_H__ */
