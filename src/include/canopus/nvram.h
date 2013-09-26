#ifndef _CANOPUS_MM_NVRAM_H_
#define _CANOPUS_MM_NVRAM_H_

#include <canopus/subsystem/subsystem.h>

#include <canopus/subsystem/platform.h>
#include <canopus/subsystem/mm.h>
#include <canopus/subsystem/cdh.h>
#include <canopus/subsystem/aocs/aocs.h>
#include <canopus/subsystem/power.h>
#include <canopus/subsystem/payload.h>
#include <canopus/subsystem/thermal.h>

#define NVRAM_VERSION_CURRENT 11

typedef struct nvram_t {
    nvram_header_t hdr;
	nvram_bootloader_t bootloader;
	nvram_platform_t platform;
	nvram_cdh_t cdh;
	nvram_aocs_t aocs;
	nvram_mm_t mm;
	nvram_power_t power;
	nvram_payload_t payload;
	nvram_thermal_t thermal;
} nvram_t;

extern nvram_t nvram;

#endif
