#include <canopus/assert.h>
#include <canopus/types.h>
#include <canopus/drivers/i2c.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

static xSemaphoreHandle i2c_xMutex = NULL;

void
i2c_lock_init()
{
    if (NULL == i2c_xMutex) {
        i2c_xMutex = xSemaphoreCreateRecursiveMutex();
    }
}

retval_t
i2c_lock(uint16_t timeout)
{
    assert(NULL != i2c_xMutex);
    if (pdFALSE == xSemaphoreTakeRecursive( i2c_xMutex, timeout )) {
        return RV_TIMEOUT;
    }

    return RV_SUCCESS;
}

void
i2c_unlock()
{
    assert(NULL != i2c_xMutex);
    (void)xSemaphoreGiveRecursive( i2c_xMutex );
}
