#ifndef _MAGNETORQUER_DRIVER_H_
#define _MAGNETORQUER_DRIVER_H_
#include <stdint.h>
#include <canopus/subsystem/aocs/algebra.h>

#define ABS(x)  (((x)<0)?-(x):(x))
#define E_NO_ERR -1

#define PWM_CHANNEL_1	0
#define PWM_CHANNEL_2	1
#define PWM_CHANNEL_3	2

void pwm_init(void);
void pwm_set_duty_vec(const vectorf_t);
void pwm_set_duty(const uint8_t channel, const float duty);
void pwm_enable(const uint8_t channel);
void pwm_disable(const uint8_t channel);
void pwm_allon(void);
void pwm_alloff(void);


#endif

