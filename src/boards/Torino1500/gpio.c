#include <canopus/types.h>
#include <canopus/drivers/gpio.h>
#include <canopus/board/channels.h>
#include <canopus/subsystem/aocs/aocs.h>
#include "gio.h"
#include "het.h"



#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#define GPIO_COUNT (7+4+1)

typedef enum {
	GIO, HET
} mode_e;

static const struct {
	mode_e mode;
	void *base;
	/*
	union {
		hetBASE_t *het;
		gioBASE_t *gio;
	} reg;
	*/
	uint8_t bit;
} config[GPIO_COUNT] = { // XXX this is Torino1000
#if 0
		{ HET, (void *)hetREG1, 1},	// GPIO_0
		{ HET, (void *)hetREG1, 7}, // GPIO_1
		{ HET, (void *)hetREG1, 9}, // GPIO_2
		{ HET, (void *)hetREG1, 4}, // GPIO_3
		{ HET, (void *)hetREG1, 15}, // GPIO_4
		// FIXME missing GPIO_5?
		{ HET, (void *)hetREG1, 14}, // GPIO_6
		{ HET, (void *)hetREG1, 16}, // GPIO_7
		{ HET, (void *)hetREG1, 18}, // Aux0
		{ GIO, (void *)gioPORTA, 5}, // Led0
#endif
		{ GIO, (void *)gioPORTA, 2}, // ActiveTMS
		{ GIO, (void *)gioPORTA, 6}, // Led1 / Aux1
		{ GIO, (void *)gioPORTA, 7}, // Led2 / Aux2
};

retval_t
GPIO_Init(gpio_id_t id)
{
	static int initialized = 0;

	if (!initialized) {
		gioInit();
		hetInit();
		initialized = 1;
	}

	return RV_SUCCESS;
}

retval_t
GPIO_Set(gpio_id_t id, gpio_state_t state)
{
	uint8_t bit;

	if (id >= GPIO_COUNT || NULL == config[id].base) {
		return RV_ILLEGAL;
	}
	bit = config[id].bit;

	if (GIO == config[id].mode) {
		gioPORT_t *gio = (gioPORT_t *)config[id].base;

		if (GPIO_HIGH == state) {
			gio->DSET = 1U << bit;
		} else {
			gio->DCLR = 1U << bit;
		}
	} else {
		hetBASE_t *het = (hetBASE_t *)config[id].base;

		if (GPIO_HIGH == state) {
			het->DSET = 1 << bit;
		} else {
			het->DCLR = 1 << bit;
		}
	}

	return RV_SUCCESS;
}



#pragma CODE_STATE(gioLowLevelInterrupt, 32)
#pragma INTERRUPT(gioLowLevelInterrupt, IRQ)

void gioLowLevelInterrupt(void)
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

    sint32 offset = gioREG->OFFSET1 - 1U;

	if (offset >= 8)
	{
		gioNotification(gioPORTB, offset-8);
	}
    else
    {
    	if (offset == GPIO_DREADY_BIT){
    		if(imu_dready_event()){
    			xSemaphoreGiveFromISR(xSemaphore_imu_dready, &xHigherPriorityTaskWoken );
    		}
    	}
    }
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
