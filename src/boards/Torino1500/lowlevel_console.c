#include <sci.h>
#include <canopus/drivers/console_lowlevel.h>

#ifdef DEBUG_CONSOLE_ENABLED

#define CONSOLE_REG sciREG

void
lowlevel_console_putchar(char c)
{
	// FIXME disable IRQ?
	sciSendByte(CONSOLE_REG, c);
	// FIXME enable IRQ?
}

#endif /* DEBUG_CONSOLE_ENABLED */
