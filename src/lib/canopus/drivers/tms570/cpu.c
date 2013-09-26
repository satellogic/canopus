#include <canopus/cpu.h>

#include "system.h"

extern void _c_int00(); /* sys_startup.c */

static inline void
cpu_system_reset()
{
    /* Setting RESET1 [...] causes a system software reset. */
    systemREG1->SYSECR = 1 << 15;
}

static inline void
cpu_soft_reset()
{
    /* Only the CPU is reset whenever this bit is toggled. There is no system reset. */
    systemREG1->MMUGCR ^= 1;
}

/* A CPU reset must be issued after the memory swap key has been changed for the memory swap to occur. */
void
cpu_map_sram_at_0x00000000(void)
{
    systemREG1->BMMCR1 = 0x5;
}
void
cpu_map_flash_at_0x00000000(void)
{
    systemREG1->BMMCR1 = 0xA;
}

void
cpu_map_flash_at_0x00000000_and_reset()
{
    cpu_map_flash_at_0x00000000();

    cpu_soft_reset();
}

void
cpu_reset0()
{
	cpu_map_flash_at_0x00000000();

	cpu_system_reset();

	// in case...
	cpu_soft_reset();

	// in case...
    _c_int00();
}

void cpu_reset2(const char *reason, int errcode)
{
	//FIXME lame call, errcode discarded :/
	cpu_reset(reason);
}
