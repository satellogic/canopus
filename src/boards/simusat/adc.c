#include <canopus/board/adc.h>
#include <canopus/types.h>

#define ADC_BASE                               adcREG1
#define ADC_GROUP                              adcGROUP1
#define ADC_VREF                               2.5
#define ADC_VREF_AVG_ITER              16
#define ADC_DELAY_BTW_MUXING   (10 / portTICK_RATE_MS)
#define ADC_ILLEGAL_BANK               0xFF

typedef struct adcData_t {
	uint16_t value;
} adcData_t;

static adcData_t adc_data[ADC_CHANNELS];
static float vref;

void  adc_init() {
	vref = 2.5/(3.3/4095);
}

static inline float adc_to_volts(const uint16_t val) {
    return val * ADC_VREF / vref;
}

void adc_select_bank(const uint8_t bank) {

}

void adc_get_samples(const uint8_t bank, float *samples){
       int i;
       adc_select_bank(bank);

       adc_data[0].value = 128;
       adc_data[1].value = 256;
       adc_data[2].value = 512;
       adc_data[3].value = 1024;
       adc_data[4].value = vref;
       adc_data[5].value = 1.9 * (vref / ADC_VREF);
       adc_data[6].value = 1.9 * (vref / ADC_VREF);
       adc_data[7].value = 92;
       adc_data[8].value = 25;
       adc_data[9].value = 40;
       adc_data[10].value = 89;
       adc_data[11].value = 4095;

       for (i=0; i < ADC_CHANNELS; ++i){
               samples[i] =  adc_to_volts(adc_data[i].value);
       }
}

retval_t adc_get_25vref_stats(frame_t *oframe)
{
	frame_put_u16(oframe, 254);
	frame_put_u16(oframe, 158);
	frame_put_u16(oframe, 100);

	return RV_SUCCESS;
}
