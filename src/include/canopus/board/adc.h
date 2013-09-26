#ifndef _CANOPUS_DRIVERS_BOARD_ADC_H
#define _CANOPUS_DRIVERS_BOARD_ADC_H

#include <canopus/types.h>
#include <canopus/frame.h>

#define ADC_BANK_TEMP                  0x01
#define ADC_BANK_SOLAR                 0x02

#define ADC_CHANNELS                   15

#define ADC_CHANNEL_VREF_25			   (13)

/* TEMPERATURE BACK */
//#define ADC_CHANNEL_TEMP1		0
//#define ADC_CHANNEL_TEMP2		1
//#define ADC_CHANNEL_TEMP3		2
//#define ADC_CHANNEL_TEMP4		3
//#define ADC_CHANNEL_TEMP5		4
//#define ADC_CHANNEL_TEMP6		5
//#define ADC_CHANNEL_TEMP7		6
//#define ADC_CHANNEL_TEMP8		7
//#define ADC_CHANNEL_TEMP9		8
#define ADC_CHANNEL_GND1		12
#define ADC_CHANNEL_VREF		13
#define ADC_CHANNEL_GND2		14

/* SOLAR is mapped tu IMU axes. AOCS uses IMU axes */

#define ADC_CHANNEL_SOLAR_X_POS			   (7)
#define ADC_CHANNEL_SOLAR_X_NEG			   (8)

#define ADC_CHANNEL_SOLAR_Y_POS			   (4)
#define ADC_CHANNEL_SOLAR_Y_NEG			   (3)

#define ADC_CHANNEL_SOLAR_Z_POS			   (11)
#define ADC_CHANNEL_SOLAR_Z_NEG			   (12)


#define ADC_CHANNEL_TEMP_X_POS			   (5)
#define ADC_CHANNEL_TEMP_X_NEG			   (6)

#define ADC_CHANNEL_TEMP_Y_POS			   (9)
#define ADC_CHANNEL_TEMP_Y_NEG			   (10)

#define ADC_CHANNEL_TEMP_Z_POS			   (13)
#define ADC_CHANNEL_TEMP_Z_NEG			   (14)

#define ADC_ERROR_VALUE						-1000.
void adc_init(void);

void adc_get_samples(uint8_t bank, float *samples_v);

retval_t adc_get_25vref_stats(frame_t * oframe);

#endif /* _CANOPUS_DRIVERS_BOARD_ADC_H */
