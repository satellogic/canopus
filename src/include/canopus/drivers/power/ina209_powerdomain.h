#ifndef _INA209_POWERDOMAIN_H_
#define _INA209_POWERDOMAIN_H_

#include <canopus/types.h>
#include <canopus/drivers/channel.h>
#include <canopus/drivers/power/ina209.h>

retval_t PD_ina_initialize(const channel_t *const channel, float voltage, float criticalPowerLevel_mW);
retval_t PD_ina_turnOn(const channel_t *const channel);
retval_t PD_ina_turnOff(const channel_t *const channel);

#endif /* _INA209_POWERDOMAIN_H_ */
