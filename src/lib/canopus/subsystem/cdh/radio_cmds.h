#ifndef _CANOPUS_SS_RADIO_CMDS_H
#define _CANOPUS_SS_RADIO_CMDS_H

#include <canopus/types.h>
#include <canopus/subsystem/subsystem.h>
#include <canopus/frame.h>

retval_t cmd_radio_set_tx_power(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_set_beacon_interval(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_set_beacon_data(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_get_config(const subsystem_t *self, frame_t * iframe, frame_t *oframe);

retval_t cmd_radio_set_config(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_flash_config(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_send_frame(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_get_telemetry(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_send_frame_bps(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_set_tx_bps(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

retval_t cmd_radio_set_rx_bps(const subsystem_t *self, frame_t * iframe, frame_t * oframe);

#endif
