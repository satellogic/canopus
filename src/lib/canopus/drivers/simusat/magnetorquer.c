#include <canopus/drivers/magnetorquer.h>
#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/logging.h>

#include <math.h>

void pwm_init(void) {
    log_report(LOG_MTQ, "PWM INIT\n");
}

void pwm_set_duty_vec(const vectorf_t dutyin)
{
    vectorf_t duty;
    VCOPY(duty, dutyin);

    pwm_set_duty(0, duty[0]);
    pwm_set_duty(1, duty[1]);
    pwm_set_duty(2, duty[2]);
}

void pwm_alloff(void)
{
    int i;
    for(i=0;i<3;i++) pwm_disable(i);
}

void pwm_allon(void)
{
    int i;
    for(i=0;i<3;i++) pwm_enable(i);
}

/* duty Duty cycle, 0=0%, 255=100%
   channel=0,1,2 */
void pwm_set_duty(const uint8_t channel, const float duty) {
	log_report_fmt(LOG_MTQ_VERBOSE, "PWM Channel:%d Duty:%.2f\n",channel,duty);
}

/* channel = 0,1,2 */
/* This functions must work when called with pwm running or stopped */
void pwm_enable(const uint8_t channel) {
//	log_report_fmt(LOG_MTQ_VERBOSE, "PWM ENABLED Channel:%d\n",channel);
}

/* channel = 0,1,2 */
/* This functions must work when called with pwm running or stopped */
void pwm_disable(const uint8_t channel) {
//    log_report_fmt(LOG_MTQ_VERBOSE, "PWM DISABLED Channel:%d\n",channel);
}

