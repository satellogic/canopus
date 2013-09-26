#include "het.h"
#include <canopus/types.h>
#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/drivers/tms570/pwm.h>
#include <canopus/drivers/magnetorquer.h>
#include "assert.h"

#define PWM_HETRAM		hetRAM1
#define PWM_1A			0
#define PWM_1B			1
#define PWM_2A			2
#define PWM_2B			3
#define PWM_3A			4
#define PWM_3B			5

static const uint8_t pwms[] = {PWM_1A, PWM_1B, PWM_2A,PWM_2B, PWM_3A, PWM_3B };

static inline uint8_t pwm_get_channel_A(const uint8_t channel) {
	switch (channel) {
		case PWM_CHANNEL_1:
			return PWM_1A;
		case PWM_CHANNEL_2:
			return PWM_2A;
		case PWM_CHANNEL_3:
			return PWM_3A;
		default:
			assert(0);
	}
	return 0;
}

static inline uint8_t pwm_get_channel_B(const uint8_t channel) {
	switch (channel) {
		case PWM_CHANNEL_1:
			return PWM_1B;
		case PWM_CHANNEL_2:
			return PWM_2B;
		case PWM_CHANNEL_3:
			return PWM_3B;
		default:
			assert(0);
	}
	return 0;
}

void pwm_set_duty(const uint8_t channel, const float duty) {
	if (duty < 0) {
		pwmSetDuty(PWM_HETRAM, pwm_get_channel_A(channel), (uint32)-duty);
		pwmSetDuty(PWM_HETRAM, pwm_get_channel_B(channel), 0);
	} else {
		pwmSetDuty(PWM_HETRAM, pwm_get_channel_A(channel), 0);
		pwmSetDuty(PWM_HETRAM, pwm_get_channel_B(channel), (uint32)duty);
	}
}

void pwm_set_duty_vec(const vectorf_t duty_vec) {
	int i;
	for (i=0; i < 3; ++i){
		pwm_set_duty(i, duty_vec[i]);
	}
}

void pwm_alloff() {
	int i;
	for (i=0; i < ARRAY_COUNT(pwms); ++i) {
		pwmStop(PWM_HETRAM, pwms[i]);
	}
}

void pwm_allon() {
	int i;
	for (i=0; i < ARRAY_COUNT(pwms); ++i) {
		pwmStart(PWM_HETRAM, pwms[i]);
	}
}
void pwm_disable(const uint8_t channel) {
	pwmStop(PWM_HETRAM, pwm_get_channel_A(channel));
	pwmStop(PWM_HETRAM, pwm_get_channel_B(channel));
}

void pwm_enable(const uint8_t channel) {
	pwmStart(PWM_HETRAM, pwm_get_channel_A(channel));
	pwmStart(PWM_HETRAM, pwm_get_channel_B(channel));
}
void pwm_init() {
	pwm_alloff();
	pwm_set_duty_vec((vectorf_t){0, 0, 0});
}

