#ifndef _CANOPUS_DRIVERS_ADC_H
#define _CANOPUS_DRIVERS_ADC_H

#include <canopus/types.h>
#include <canopus/assert.h>

typedef struct ADC_convertionValue_t {
    float factor;
    float offset;
} ADC_convertionValue_t;

static inline float ADC_convertion(const ADC_convertionValue_t ADC_convertionTable[], uint8_t channel, uint16_t rawValue) {
	ADC_convertionValue_t convert;

#ifdef BROKEN_ASSERT_ON_SAM7_FIXED
	assert(channel < ARRAY_COUNT(ADC_convertionTable));
#endif
	convert = ADC_convertionTable[channel];
	return convert.offset + convert.factor * rawValue;
}

#define ADC_ERROR_VALUE ((uint16_t) -1)

#endif /* _CANOPUS_DRIVERS_ADC_H */
