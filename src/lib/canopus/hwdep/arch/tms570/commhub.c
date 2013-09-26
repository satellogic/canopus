#include <canopus/drivers/leds.h>

void
commhub_kick()
{
	led_toggle(1); /* triggers the Alive watchdog */
}
