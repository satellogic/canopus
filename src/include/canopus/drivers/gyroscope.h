#ifndef _CANOPUS_GYROSCOPE_H_
#define _CANOPUS_GYROSCOPE_H_

#include <canopus/time.h>
#include <canopus/drivers/device_driver.h>

/* Some forward declarations */
typedef struct gyroscope gyroscope_t;


/**
 * Public interface to interact with any gyroscope.
 */


/** Common configuration for every gyroscope */
typedef struct {
    /* Underlying device driver */
    const device_driver_t * devdrv;
    const int16_t x_bias;
    const float x_scale;
    const int16_t y_bias;
    const float y_scale;
    const int16_t z_bias;
    const float z_scale;
} gyroscope_config_t;

retval_t gyroscope_initialize(
            const gyroscope_t * const gyroscope,
            const gyroscope_config_t * const config);
retval_t gyroscope_deinitialize(
            const gyroscope_t * const gyroscope);

retval_t gyroscope_get_temp(
            const gyroscope_t *gyroscope,
            float * temp_out_C,
            timespec_t *timestamp);
retval_t gyroscope_get_xyz(
            const gyroscope_t * const gyroscope,
            float * const xout,
            float * const yout,
            float * const zout,
            timespec_t * const timestamp);
retval_t gyroscope_get_xyz_raw(
            const gyroscope_t * const gyroscope,
            int16_t * const xout,
            int16_t * const yout,
            int16_t * const zout,
            timespec_t * const timestamp);
retval_t gyroscope_compensate_temp_drift(
            const gyroscope_t *gyroscope);

/**
 *  Support interface for Gyroscope implementors.
 */

/** Common "class" (as in Object Oriented Programming) for a particular
 *  gyroscope implementation, to be shared among every instance of that
 *  implementation */
typedef struct {
    /* To be called from `gyroscope_deinitialize()` */
    retval_t (* const initialize)(
            const gyroscope_t * const gyroscope,
            const gyroscope_config_t * const config);
    /* To be called from `gyroscope_deinitialize()` */
    retval_t (* const deinitialize)(
            const gyroscope_t * const gyroscope);
    /* To be called from `gyroscope_get_xyz()` */
    retval_t (* const get_xyz)(
            const gyroscope_t * const gyroscope,
            float * const xout,
            float * const yout,
            float * const zout,
            timespec_t * const timestamp);
    /* To be called from `gyroscope_get_xyz()` */
    retval_t (* const get_xyz_raw)(
            const gyroscope_t * const gyroscope,
            int16_t * const xout,
            int16_t * const yout,
            int16_t * const zout,
            timespec_t * const timestamp);
    /* To be called from `gyroscope_get_temp()` */
    retval_t (* const get_temp)(
            const gyroscope_t * gyroscope,
            float *temp_out_C,
            timespec_t *timestamp);
    retval_t (* const compensate_temp_drift)(
            const gyroscope_t * gyroscope);

} gyroscope_base_t;


/** State "class" (as in Object Oriented Programming) for a particular
 *  gyroscope implementation. This state will be unique per each
 *  instance of a gyroscope implementation. */
typedef struct {
    bool is_initialized;
    const device_driver_t * devdrv;
    int16_t x_bias;
    float x_scale;
    int16_t y_bias;
    float y_scale;
    int16_t z_bias;
    float z_scale;
} gyroscope_state_t;


/** Every instance of a gyroscope implementation will be represented by
 *  this product type between the common base implementation and individual
 *  state */
struct gyroscope {
    const gyroscope_base_t * const base;
    gyroscope_state_t      * const state;
};


#define GYROSCOPE(base, state) \
    { base, (gyroscope_state_t*)state }

#endif
