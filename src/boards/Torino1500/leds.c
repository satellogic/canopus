#include <canopus/drivers/leds.h>
#include <stdint.h>

#include "gio.h"

#define LED_MAX 2

static int gio_bit[LED_MAX] = { /* Torino1500: PORTA */
    5, /* Led0 */
    7, /* Alive */
};

retval_t
leds_init()
{
	//gioInit();

	return RV_SUCCESS;
}

retval_t
led_set(uint32_t led_id, led_state_t state)
{
	if (led_id >= LED_MAX) {
		return RV_ILLEGAL;
	}

	switch (state) {
	case LED_ON:
		gioSetBit(gioPORTA, gio_bit[led_id], 1);
		break;
	case LED_OFF:
		gioSetBit(gioPORTA, gio_bit[led_id], 0);
		break;
	}

	return RV_SUCCESS;
}

retval_t
led_toggle(uint32_t led_id)
{
	if (led_id >= LED_MAX) {
		return RV_ILLEGAL;
	}

	gioToggleBit(gioPORTA, gio_bit[led_id]);

	return RV_SUCCESS;
}
