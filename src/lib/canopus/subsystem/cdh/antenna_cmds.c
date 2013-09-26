#include <canopus/subsystem/subsystem.h>

#include <canopus/subsystem/power.h> // POWER_turn_switch_on/off
#include <canopus/nvram.h>

#include "antenna_cmds.h"

retval_t cmd_antenna_deploy_inhibit(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t inhibit;
	retval_t rv;

	rv = frame_get_u8(iframe, &inhibit);
	if (RV_SUCCESS != rv) return rv;

	nvram.cdh.antenna_deploy_enabled = inhibit ? false : true;
	/* Not flushing to nvram on purpose, we may want this command to only act temporarily */
    log_report_fmt(LOG_SS_CDH_ANTENNA, "ANTENNA: antenna_deploy_enabled is now %s\n", inhibit ? "FALSE" : "TRUE");

	return rv;
}

