#include <canopus/subsystem/subsystem.h>

#include <canopus/drivers/radio/lithium.h>

#include <canopus/md5.h>
#include <canopus/nvram.h>

#include "radio_cmds.h"

retval_t cmd_radio_set_tx_power(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t power;
	if (RV_SUCCESS != frame_get_u8(iframe, &power)) return RV_NOSPACE;

	return lithium_set_tx_power(power);
}

retval_t cmd_radio_set_beacon_interval(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	uint8_t interval_s;
	if (RV_SUCCESS != frame_get_u8(iframe, &interval_s)) return RV_NOSPACE;

	return lithium_set_beacon_interval(interval_s);
}

retval_t cmd_radio_set_beacon_data(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_t aux;

	frame_copy(&aux, iframe);	/* A copy is needed, because lithium_set_beacon disposes the frame */

	frame_advance(iframe, _frame_available_data(iframe));
	return lithium_set_beacon_data(&aux);
}

retval_t cmd_radio_get_config(const subsystem_t *self, frame_t * iframe, frame_t *oframe) {
	lithium_configuration_t *config;
	retval_t rv;

	if (RV_SUCCESS != frame_get_data_pointer(oframe, (void **)&config, sizeof(*config))) {
		return RV_NOSPACE;
	}
	frame_advance(oframe, sizeof(*config));
	rv = lithium_get_configuration(config);
	return rv;
}

retval_t cmd_radio_set_config(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	lithium_configuration_t *config;
	retval_t rv;

	if (RV_SUCCESS != frame_get_data_pointer(iframe, (void **)&config, sizeof(*config))) {
		return RV_NOSPACE;
	}
	rv = lithium_set_configuration(config);
    frame_put_u8(oframe, rv);

	frame_advance(iframe, sizeof(*config));
	return rv;
}

retval_t cmd_radio_flash_config(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    retval_t rv;
    void *md5;

    if (RV_SUCCESS != frame_get_data_pointer(iframe, &md5, sizeof(md5))) {
        return RV_NOSPACE;
    }

    frame_advance(iframe, MD5_DIGEST_SIZE);
    rv = lithium_write_configuration_to_flash(md5);
    frame_put_u8(oframe, rv);

    return cmd_radio_get_config(self, iframe, oframe);
}

retval_t cmd_radio_send_frame(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	frame_t aux;

	frame_copy(&aux, iframe);	/* A copy is needed, because lithium_send_data disposes the frame */

	frame_advance(iframe, _frame_available_data(iframe));
	return lithium_send_data(&aux);
}

retval_t cmd_radio_get_telemetry(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	lithium_telemetry_t *telemetry;

	if (RV_SUCCESS != frame_get_data_pointer(oframe, (void **)&telemetry, sizeof(*telemetry))) {
		return RV_NOSPACE;
	}
	frame_advance(oframe, sizeof(*telemetry));
	return lithium_get_telemetry(telemetry);
}

/* (manu) lithium needs to be treated gently...
 * (phil) may be related to lithium_workaround_after_reconfiguring_anything()
 */
#define LITHIUM_BLACKMAGIC_SENDDATA_DELAY_MS 100

retval_t cmd_radio_send_frame_bps(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    lithium_configuration_t config;
    uint8_t tx_rf_baud_rate;
    retval_t rv;
    frame_t aux;

    /* save current config for restoring tx_bps setting */
    rv = lithium_get_configuration(&config);
    if (RV_SUCCESS != rv) return rv;

    rv = frame_get_u8(iframe, &tx_rf_baud_rate);
    if (RV_SUCCESS != rv) return rv;

    vTaskDelay(LITHIUM_BLACKMAGIC_SENDDATA_DELAY_MS / portTICK_RATE_MS);

    frame_copy(&aux, iframe); /* A copy is needed, because lithium_send_data disposes the frame */
    frame_advance(iframe, _frame_available_data(iframe));

    rv = lithium_set_tx_bps(tx_rf_baud_rate);
    if (RV_SUCCESS != rv) return rv;

    vTaskDelay(LITHIUM_BLACKMAGIC_SENDDATA_DELAY_MS / portTICK_RATE_MS);

    lithium_send_data(&aux);

    vTaskDelay(LITHIUM_BLACKMAGIC_SENDDATA_DELAY_MS / portTICK_RATE_MS);
    
    return lithium_set_tx_bps(config.tx_rf_baud_rate);
}

retval_t cmd_radio_set_tx_bps(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
    /* FIXME: this should give an answer in oframe */
    uint8_t tx_rf_baud_rate;
    retval_t rv;

    rv = frame_get_u8(iframe, &tx_rf_baud_rate);
    if (RV_SUCCESS != rv) return rv;

    return lithium_set_tx_bps(tx_rf_baud_rate);
}

retval_t cmd_radio_set_rx_bps(const subsystem_t *self, frame_t * iframe, frame_t * oframe) {
	/* TODO: finish cmd_radio_set_rx_bps() */
	return RV_NOTIMPLEMENTED;
}
