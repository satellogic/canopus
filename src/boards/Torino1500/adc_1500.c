#include <canopus/drivers/commhub_1500.h>
#include <canopus/board/adc.h>
#include <canopus/frame.h>
#include "stdint.h"
#include "adc.h"
#include "assert.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#define ADC_BASE					adcREG1
#define ADC_GROUP					adcGROUP1
#define ADC_VREF					2.5
#define ADC_VREF_AVG_ITER			16
#define ADC_DELAY_BTW_MUXING		(10 / portTICK_RATE_MS)
#define ADC_ILLEGAL_BANK			0xFF
#define ADC_AMUX_TMS0_MUX_ENABLE	1
#define ADC_OCTOPUS_MUX_MASK		(1<<8)
#define ADC_VREF_CHANNEL			1
#define ADC_RECALIBRATION_MS		10000

static adcData_t adc_data[ADC_CHANNELS];
static float vref;
static uint16_t last_vref_measured, biggest_vref_measured, smallest_vref_measured;
static portTickType last_calibration_tick;
static uint8_t actual_bank = ADC_ILLEGAL_BANK;

static xSemaphoreHandle adc_xMutex = NULL;

static void
adc_lock_init()
{
    if (NULL == adc_xMutex) {
    	adc_xMutex = xSemaphoreCreateRecursiveMutex();
    }
}

retval_t
adc_lock(uint16_t timeout)
{
    assert(NULL != adc_xMutex);
    if (pdFALSE == xSemaphoreTakeRecursive(adc_xMutex, timeout)) {
        return RV_TIMEOUT;
    }

    return RV_SUCCESS;
}

void
adc_unlock()
{
    assert(NULL != adc_xMutex);
    (void)xSemaphoreGiveRecursive(adc_xMutex);
}

static
void adc_select_bank(const uint8_t bank) {
	if (actual_bank != bank) {
		actual_bank = bank;
		commhub_writeRegister(COMMHUB_REG_AMUX, ADC_AMUX_TMS0_MUX_ENABLE);
		if (ADC_BANK_TEMP == bank) {
			commhub_andRegister(COMMHUB_REG_GP_OUT, ~ADC_OCTOPUS_MUX_MASK);
		} else {
			commhub_orRegister(COMMHUB_REG_GP_OUT, ADC_OCTOPUS_MUX_MASK);
		}
		vTaskDelay(ADC_DELAY_BTW_MUXING);
	}
}

static
void adc_calibrate(void){
	int i;
	adcCalibration(ADC_BASE);
	adcMidPointCalibration(ADC_BASE);
	// set mux to the one that has the VREF=2.5V
	adc_select_bank(ADC_BANK_TEMP);

	// just average vref
	vref = 0;
	for(i=0; i < ADC_VREF_AVG_ITER; ++i){

		adcStartConversion(ADC_BASE, ADC_GROUP);
		while(!adcIsConversionComplete(ADC_BASE, ADC_GROUP));
		adcGetData(ADC_BASE, ADC_GROUP, adc_data);

		last_vref_measured = adc_data[ADC_VREF_CHANNEL].value;
		vref += (float) last_vref_measured;

		if (last_vref_measured > biggest_vref_measured) {
			biggest_vref_measured = last_vref_measured;
		}
		if (last_vref_measured < smallest_vref_measured) {
			smallest_vref_measured = last_vref_measured;
		}
	}
	vref /= ADC_VREF_AVG_ITER;

}

void  adc_init() {
	smallest_vref_measured = UINT16_MAX;
	biggest_vref_measured = 0;
	adc_lock_init();
	adc_calibrate();
	last_calibration_tick = xTaskGetTickCount();
}

static inline float adc_to_volts(const uint16_t val) {
	return val * ADC_VREF / vref;
}


void adc_get_samples(const uint8_t bank, float *samples_v) {
	int i;
	portTickType current_tick;

	adc_lock(100/portTICK_RATE_MS);
	current_tick = xTaskGetTickCount();
	if (((current_tick - last_calibration_tick) / portTICK_RATE_MS) > ADC_RECALIBRATION_MS){
		last_calibration_tick = current_tick;
		adc_calibrate();
	}

	adc_select_bank(bank);
	adcStartConversion(ADC_BASE, ADC_GROUP);
	while(!adcIsConversionComplete(ADC_BASE, ADC_GROUP));

	adcGetData(ADC_BASE, ADC_GROUP, adc_data);

	for (i=0; i < ADC_CHANNELS; ++i){
		// there is something strange, adc pins are inverted
		samples_v[(ADC_CHANNELS-1)-i] =  adc_to_volts(adc_data[i].value);
	}
	adc_unlock();
}

retval_t adc_get_25vref_stats(frame_t * oframe) {
	retval_t rv;
	rv = frame_put_u16(oframe, last_vref_measured);
	SUCCESS_OR_RETURN(rv);
	rv = frame_put_u16(oframe, smallest_vref_measured);
	SUCCESS_OR_RETURN(rv);
	return frame_put_u16(oframe, biggest_vref_measured);
}
