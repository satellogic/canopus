#ifndef _CANOPUS_DEBUG_H_
#define _CANOPUS_DEBUG_H_

#include <canopus/types.h>

#define DEBUG_CURRENT_LINE() Console_report_fmt("\n" __FILE__ ":%d %s()\n", __LINE__, __FUNCTION__)

#ifdef USE_DEBUG_DUMP
void debug_dump(void *ptr, size_t size, const char *desc);
void debug_dumpbyte(uint8_t byte, const char *desc);
#else
#define debug_dump(p, s, d)
#define debug_dumpbyte(b, d)
#endif

#endif

