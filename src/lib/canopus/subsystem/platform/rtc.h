#ifndef __RTC_H__
#define __RTC_H__

#include <canopus/types.h>

uint64_t rtc_get_current_time();
void rtc_set_current_time(uint64_t current_time_ms);

#endif // __RTC_H__
