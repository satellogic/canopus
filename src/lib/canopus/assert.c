#include <canopus/assert.h>
#include <canopus/debug.h>
#include <canopus/cpu.h>
#include <FreeRTOS.h>
#include <task.h>

#ifndef NDEBUG
#include <canopus/drivers/console_lowlevel.h>

#include <string.h> /* DEBUG_AREA_ENABLED */

void
canopus_assert(const char *expression, const char *filename, const char *funcname, unsigned int line)
{
    FUTURE_HOOK_4(canopus_assert_0, expression, filename, funcname, line);

    portDISABLE_INTERRUPTS();

    FUTURE_HOOK_4(canopus_assert_1, expression, filename, funcname, line);
#ifdef DEBUG_AREA_ENABLED
    strncpy(debug_area->assert.filename, filename, sizeof(debug_area->info_str));
    strncpy(debug_area->assert.expression, expression, sizeof(debug_area->info_str));
    debug_area->assert.line = line;
#endif

    lowlevel_console_putnewline();
    lowlevel_console_putstring("assertion failed [");
    lowlevel_console_putstring(filename);
    lowlevel_console_putstring(":0x");
    lowlevel_console_puthex32(line);
    lowlevel_console_putchar(':');
    lowlevel_console_putstring(funcname);
    lowlevel_console_putstring("] ");
    lowlevel_console_putstring(expression);
    lowlevel_console_putnewline();
    lowlevel_console_putnewline();

    cpu_reset("ASSERT");
}

#ifdef __USE_POSIX
void
__assert_fail_canopus_style1(const char *expression,const char *filename, unsigned int line, const char *function)
{
    canopus_assert(expression, filename, function, line);
}
#endif

#ifdef __ARM_EABI__
void
__assert_fail_canopus_style2(const char *filename, int line, const char *function, const char *expression)
{
    canopus_assert(expression, filename, function, line);
}
#endif

#ifdef __TI_EABI_SUPPORT__
/* @provided_by rtsv7R4_T_be_v3D16_eabi.lib */
void _abort_msg(const char *reason)
{
    // FIXME not working
}

void
__assert_fail_canopus_style3(int valid, const char *reason)
{
    if (valid) return;

    canopus_assert(reason, "", "", 0); // TODO improve
}
#endif

#ifdef __MINGW32__
void
__assert_fail_canopus_style4(const char *reason, const char *filename, int line)
{
    canopus_assert(reason, filename, "", line); // TODO improve
}
#endif

#endif /* NDEBUG */
