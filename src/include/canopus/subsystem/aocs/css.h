#ifndef _CANOPUS_SUBSYSTEM_ADCS_CSS_H_
#define _CANOPUS_SUBSYSTEM_ADCS_CSS_H_

#include <canopus/types.h>
#include <canopus/subsystem/aocs/algebra.h>

typedef struct css_adc_measurement_volts_t {
	double x_neg;
	double x_pos;
	double y_neg;
	double y_pos;
	double z_neg;
	double z_pos;
} css_adc_measurement_volts_t;

typedef struct sun_vector_t {
	double x;
	double y;
	double z;
} sun_vector_t;

typedef struct sun_vector_bits_t {
	int16_t x;
	int16_t y;
	int16_t z;
} sun_vector_bits_t;

typedef struct sun_data_t {
	css_adc_measurement_volts_t css_adc_measurement_volts;
	sun_vector_t sun_vector;
	sun_vector_bits_t sun_vector_bits;
} sun_data_t;

#define SETZ(__posorneg, __num) do{if( __posorneg##_xyz[__num] < 0){ __posorneg##_xyz[__num] = 0;}}while(0);

#define COSINE_WITH_NORMAL(__posorneg, __num) do{\
__posorneg##_xyz[__num] = __posorneg##_xyz[__num] / sunsensor_##__posorneg##_nominal[__num];\
}while(0);

// San's ADC API Already gives measurement in volts now.
#define ADCBITS2VOLTS(__bits) ((__bits/4095.)*3.3)

void volts2sunvec(vectord_t sun, const vectord_t positive_xyz, const vectord_t negative_xyz);

void fill_up_sun_t(sun_data_t *, const css_adc_measurement_volts_t *);

#endif
