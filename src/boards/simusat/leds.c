#include <canopus/types.h>
#include <canopus/drivers/leds.h>

#include <stdint.h>
#include <stdio.h>

#define LED_MAX 4

#ifdef _DEBUG
# define STREAM stdout
#else
# define STREAM NULL
#endif

static int8_t state[LED_MAX] = { };
const static char *led_disp[] = { "off", "ON" };

retval_t
led_init(uint32_t led_id)
{
	if (led_id >= LED_MAX) {
		return RV_ILLEGAL;
	}

	return RV_SUCCESS;
}

retval_t
led_toggle(uint32_t led_id)
{
	if (led_id >= LED_MAX) {
		return RV_ILLEGAL;
	}

	state[led_id] += 1;
	state[led_id] %= 2;
	if (NULL != STREAM) {
		fprintf(STREAM, "LED#%d %s\n", led_id, led_disp[state[led_id]]);
		fflush(STREAM); /* required on win32 */
	}

	return RV_SUCCESS;
}
