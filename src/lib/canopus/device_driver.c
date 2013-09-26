#include <canopus/drivers/device_driver.h>

retval_t device_driver_initialize(
        const device_driver_t * const driver,
        const device_driver_config_t * const config)
{
    return driver->base->initialize(driver, config);
}

retval_t device_driver_deinitialize(
        const device_driver_t * const driver)
{
    return driver->base->deinitialize(driver);

}
