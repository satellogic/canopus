#ifndef _CANOPUS_DEVICE_DRIVER_H_
#define _CANOPUS_DEVICE_DRIVER_H_

#include <canopus/types.h>

#define DECLARE_DEVDRV(base, state) { base, (device_driver_state_t*)state }
#define DEVDRV_INIT(devdrv, conf) device_driver_initialize(devdrv, (device_driver_config_t*)(conf))

/* Some forward declarations */
typedef struct device_driver device_driver_t;

/**
 * Device Driver Configuration.
 *
 * Driver behaviour may be altered using read-only configuration parameters
 * from this structure on driver initialization.
 *
 * Must not be used to hold mutable state.
 *
 * This opaque structure is to be defined by the device driver implementation.
 */
struct device_driver_config; /**< opaque structure */
typedef struct device_driver_config device_driver_config_t;

/**
 * Device Driver State.
 *
 * Mutable structure holding the current state for a device driver. To be used
 * internally by the Device Driver implementation.
 *
 * This opaque structure is to be defined by the device driver implementation.
 */
struct device_driver_state; /**< opaque structure */
typedef struct device_driver_state device_driver_state_t;

/**
 * Device Driver Base.
 *
 * Each concrete hardware device is expected to implement a single instance of
 * this structure, to be later used by the higher level Device Driver Features.
 *
 * This Base structure should provide common routines for Device Drivers, and
 * may be shared among many instances of Device Driver.
 */
typedef struct {
    /* To be called from `device_driver_initialize` */
    retval_t (* const initialize)(
            const device_driver_t * const driver,
            const device_driver_config_t * const config);
    /* To be called from `device_driver_initialize` */
    retval_t (* const deinitialize)(
            const device_driver_t * const driver);
} device_driver_base_t;

/**
 * Device Driver.
 *
 * Instance of a Device Driver, tying a particular state to some predefined
 * Device Driver Base potentialy shared among many Device Driver instances.
 */
struct device_driver {
    const device_driver_base_t  * const base;
    const device_driver_state_t * const state;
};

/** Initializes a Device Driver.
 *
 *  @param[out]  driver  Device Driver to be initialized.
 *  @param[in]   config  Configuration parameters to use.
 *
 *  @return RV_SUCCESS
 *  @return RV_ERROR
 */
retval_t device_driver_initialize(
        const device_driver_t * const driver,
        const device_driver_config_t * const config);

/** Deinitializes a previously initialized Device Driver.
 *
 *  @param[out]  driver  Device Driver to be initialized.
 *
 *  @return RV_SUCCESS
 *  @return RV_ERROR
 */
retval_t device_driver_deinitialize(
        const device_driver_t * const driver);

#endif
