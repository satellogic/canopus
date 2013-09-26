#ifndef _CANOPUS_DRIVERS_I2C_MADLOCK_H
#define _CANOPUS_DRIVERS_I2C_MADLOCK_H

#include <canopus/types.h>

void i2c_lock_init(void);

retval_t i2c_lock(uint16_t timeout);

void i2c_unlock(void);

#endif
