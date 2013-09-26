#ifndef _CANOPUS_CPU_H
#define _CANOPUS_CPU_H

void cpu_reset0(void) __attribute__((__noreturn__)); /* arch/board level */
void cpu_reset(const char *reason) __attribute__((__noreturn__));
void cpu_reset2(const char *reason, int errcode) __attribute__((__noreturn__)); /* deprecated? */

#if 0
/* disable any board watchdogs */
void board_disable_watchdog(void);

/* reset all modules on a board */
void canopus_board_reset(void) __attribute__((__noreturn__));
#endif

#endif
