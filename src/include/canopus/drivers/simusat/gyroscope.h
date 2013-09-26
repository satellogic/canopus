#ifndef _POSIX_DEVICE_GYROSCOPE_H_
#define _POSIX_DEVICE_GYROSCOPE_H_

#include <canopus/types.h>
#include <canopus/drivers/device_driver.h>
#include <canopus/drivers/gyroscope.h>

extern const gyroscope_base_t posix_gyroscope_base;

typedef struct posix_gyroscope_state_t {
    bool is_initialized;
} posix_gyroscope_state_t;

#endif
