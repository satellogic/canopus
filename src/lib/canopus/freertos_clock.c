#include <FreeRTOS.h>
#include <task.h>

extern portBASE_TYPE xNumOfOverflows;

/* BEGIN xTaskGetNumOfOverflows( void )  -- ADDED BY manuel@satellogic.com */

portBASE_TYPE xTaskGetNumOfOverflows( void )
{
    portBASE_TYPE xOverflows;

    taskENTER_CRITICAL();
    {
        xOverflows = xNumOfOverflows;
    }
    taskEXIT_CRITICAL();

    return xOverflows;

}


/* END xTaskGetNumOfOverflows( void )    -- ADDED BY manuel@satellogic.com */

