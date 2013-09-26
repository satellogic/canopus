#include <canopus/types.h>
#include <canopus/drivers/console_lowlevel.h>

#ifdef DEBUG_CONSOLE_ENABLED

#include <stdio.h>

void
lowlevel_console_putchar(char c)
{
    putchar(c);
}

#endif /* DEBUG_CONSOLE_ENABLED */
