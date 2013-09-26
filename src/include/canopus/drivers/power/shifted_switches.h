#ifndef _SHIFTED_SWITCHES_H_
#define _SHIFTED_SWITCHES_H_

#include <canopus/types.h>

#define SHIFTED_SW_HEATER_8		1 << 15
#define SHIFTED_SW_HEATER_7		1 << 14
#define SHIFTED_SW_HEATER_6		1 << 13
#define SHIFTED_SW_HEATER_5		1 << 12
#define SHIFTED_SW_HEATER_4		1 << 11
#define SHIFTED_SW_HEATER_3		1 << 10
#define SHIFTED_SW_HEATER_2		1 << 9
#define SHIFTED_SW_HEATER_1		1 << 8
#define SHIFTED_SW_PD_TEST_D	1 << 7
#define SHIFTED_SW_PD_TEST_C	1 << 6
#define SHIFTED_SW_PD_TEST_B	1 << 5
#define SHIFTED_SW_PD_TEST_A	1 << 4
#define SHIFTED_SW_ANT_DPL_1	1 << 3
#define SHIFTED_SW_ANT_DPL_2	1 << 2
#define SHIFTED_SW_ANT_DPL_3	1 << 1
#define SHIFTED_SW_ANT_DPL_4	1 << 0

retval_t shifter_initialize();
retval_t shifted_setAll(uint16_t switches);
retval_t shifted_getAll(uint16_t *switches);
retval_t shifted_turnOn(uint16_t bit);
retval_t shifted_turnOff(uint16_t bit);

#endif /* _SHIFTED_SWITCHES_H_ */
