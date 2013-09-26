#ifndef _CANOPUS_SS_ANTENNA_CMDS_H
#define _CANOPUS_SS_ANTENNA_CMDS_H

#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/frame.h>

#include <canopus/drivers/channel.h>

retval_t cmd_antenna_deploy_inhibit(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

#endif
