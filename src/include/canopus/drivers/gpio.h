#ifndef _CANOPUS_DRIVERS_GPIO_H_
#define _CANOPUS_DRIVERS_GPIO_H_

#include <stdint.h>
#include <canopus/types.h>

typedef uint8_t gpio_id_t;

typedef enum {
	GPIO_HIGH,	/* non-zero */
	GPIO_LOW	/* zero */
} gpio_state_t;

typedef enum {
	GPIO_IN,
	GPIO_OUT
} gpio_direction_t;

/**
 * Board pin initialization
 * @param [in] id         GPIO pin id
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t GPIO_Init(gpio_id_t id);

/**
 * Set pin state
 * @param [in] id         GPIO pin id
 * @param [in] state      GPIO pin state
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t GPIO_Set(gpio_id_t id, gpio_state_t state);

/**
 * Get pin state
 * @param [in] id         GPIO pin id
 * @param [out] state     GPIO pin state
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t GPIO_Get(gpio_id_t id, gpio_state_t *state);

/**
 * Set pin direction
 * @param [in] id         GPIO pin id
 * @param [in] dir        GPIO pin direction
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t GPIO_SetDir(gpio_id_t id, gpio_direction_t dir);

/**
 * Get pin direction
 * @param [in] id         GPIO pin id
 * @param [out] dir       GPIO pin direction
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t GPIO_GetDir(gpio_id_t id, gpio_direction_t *dir);

/**
 * Get pin description
 * @param [in] id         GPIO pin id
 * @param [out] desc      GPIO pin description
 * @retval RV_SUCCESS, RV_ILLEGAL
 */
retval_t GPIO_GetDesc(gpio_id_t id, const char **desc);

#endif

