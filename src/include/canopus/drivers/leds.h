#ifndef _CANOPUS_DRIVERS_LEDS_H_
#define _CANOPUS_DRIVERS_LEDS_H_

#include <canopus/types.h>
#include <stdint.h>

typedef enum {
    LED_OFF,
    LED_ON
} led_state_t;

/**
 * @retval RV_SUCCESS, RV_ERROR
 */
retval_t leds_init();

/**
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t led_toggle(uint32_t led_id);

retval_t led_set(uint32_t led_id, led_state_t state);

#endif

