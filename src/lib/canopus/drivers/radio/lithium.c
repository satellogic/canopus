/*
   Astrodev Li-1 API implementation
   auteur: manuel
*/
#include <canopus/assert.h>
#include <canopus/frame.h>
#include <canopus/md5.h>
#include <canopus/logging.h>
#include <canopus/drivers/channel.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/radio/lithium.h>
#include <canopus/subsystem/cdh.h> /* CDH_process_hard_commands */

#include <FreeRTOS.h>
#include <task.h>

#include <string.h>

#include "lithium_and_cdh_mix.h"

#define LITHIUM_CMD_TIMEOUT_TICKS	(1000 / portTICK_RATE_MS)

frame_t last_beacon_data_for_workaround = DECLARE_FRAME_SPACE(MAX_FRAME_SIZE);

static
uint16_t fletcher_chksum16(const uint8_t *data, int count, uint16_t accumulator )
{
	/* taken from: http://en.wikipedia.org/wiki/Fletcher's_checksum */

   uint16_t sum1 = accumulator >> 8;
   uint16_t sum2 = accumulator & 0xFF;
   int index;

   for( index = 0; index < count; ++index )
   {
      sum1 = (sum1 + data[index]) % 256;
      sum2 = (sum2 + sum1) % 256;
   }

   return (sum1 << 8) | sum2;
}

static inline
lithium_cmd_t get_li1cmd_from_frame_nocheck(frame_t *frame) {
	return (lithium_cmd_t)frame_get_u8_nocheck(frame);
}

retval_t create_command_frame(lithium_cmd_t command, frame_t *payload_frame, frame_t *frame) {
    uint16_t chksum;
    retval_t rv;
    uint16_t payload_size;

    assert(frame != NULL);

    if (payload_frame != NULL && payload_frame->size > FRAME_PAYLOAD_MAXSIZE) {
        return RV_ILLEGAL;
    }

    /* sync header */
    frame_put_u8(frame, LITHIUM_SYNC_CHAR_1);
    frame_put_u8(frame, LITHIUM_SYNC_CHAR_2);

    frame_put_u8(frame, LITHIUM_DIRECTION_INPUT); /* command 'direction'. we will always send 'input' (0x10) commands to the radio */
    frame_put_u8(frame, command);                 /* command id */

    payload_size = payload_frame == NULL ? 0 : _frame_available_data(payload_frame);
    frame_put_u16(frame, payload_size);  /* payload size */

    /* calculate and insert checksum for header */
    chksum = fletcher_chksum16(&(frame->buf[2]), 4, 0);
    frame_put_u16(frame, chksum);

    /* if no payload, we're done */
    if (payload_size == 0) {
        return RV_SUCCESS;
    }

    rv = frame_transfer(frame, payload_frame);
    if (RV_SUCCESS != rv) return RV_NOSPACE;

    /* calculate ans insert checksum for whole command */
    chksum = fletcher_chksum16(&(frame->buf[6]), payload_size + CHECKSUM_SIZE, chksum);
    frame_put_u16(frame, chksum);

    return RV_SUCCESS;
}

retval_t lithium_parse_command_frame(frame_t *frame, lithium_cmd_t *command, bool *is_ack) {

    uint16_t p_size, checksum;
    uint8_t command_direction;
    void *tmp_ptr;
    uint8_t tmp_checksum_buf[4];

    assert(frame != NULL);

    if (!frame_hasEnoughData(frame, FRAME_HEADER_SIZE)) {
        return RV_NOSPACE;  /* frame incomplete, need moar data */
    }

    if (frame_get_u8_nocheck(frame) != LITHIUM_SYNC_CHAR_1 || frame_get_u8_nocheck(frame) != LITHIUM_SYNC_CHAR_2) {
        return RV_ILLEGAL; /* not sync chars, illegal */
    }

    command_direction = frame_get_u8_nocheck(frame);
    if (command_direction != LITHIUM_DIRECTION_OUTPUT) {
        return RV_ILLEGAL; /* we should not get commands with 'direction' other than output */
    }

    *command = get_li1cmd_from_frame_nocheck(frame);

    p_size = frame_get_u16_nocheck(frame);

    tmp_checksum_buf[0] = command_direction;
    tmp_checksum_buf[1] = *command;
    tmp_checksum_buf[2] = p_size >> 8;
    tmp_checksum_buf[3] = p_size & 0xFF;

    /* check header checksum */
    checksum = fletcher_chksum16(tmp_checksum_buf, 4, 0);
    if (checksum != frame_get_u16_nocheck(frame)) {
        return RV_ILLEGAL;
    }

    if (p_size == LITHIUM_RESPONSE_ACK || p_size == LITHIUM_RESPONSE_NACK) {
        *is_ack = (p_size == LITHIUM_RESPONSE_ACK);
        /* no payload, we done */
        return RV_SUCCESS;
    }
    else if (p_size > 255) {
        return RV_ILLEGAL;
    }

    if (!frame_hasEnoughData(frame, p_size + CHECKSUM_SIZE)) {
        /* frame incomplete */
        return RV_NOSPACE;
    }

    frame_get_data_pointer(frame, &tmp_ptr, p_size);
    frame_advance(frame, p_size);

    /* check payload checksum
       we need to include the previous checksum bytes
    */
    tmp_checksum_buf[0] = checksum >> 8;
    tmp_checksum_buf[1] = checksum & 0xFF;

    checksum = fletcher_chksum16(tmp_checksum_buf, 2, checksum);
    checksum = fletcher_chksum16(tmp_ptr, p_size, checksum);

    int frame_checksum = frame_get_u16_nocheck(frame);

    if (checksum != frame_checksum) {
        return RV_ILLEGAL;
    }

    return RV_SUCCESS;
}

retval_t lithium_push_incoming_data(const frame_t *frame) {
	frame_t *dropped;
    if (pdFALSE == xQueueSendToBack(LITHIUM_STATE.queue_data, &frame, 0)) {
    	(void)xQueueReceive(LITHIUM_STATE.queue_data, &dropped, 0);
        log_report(LOG_RADIO, "Dropping a frame from queue_data\n");
    	frame_dispose(dropped);
    	if (pdTRUE == xQueueSendToBack(LITHIUM_STATE.queue_data, &frame, 1000 / portTICK_RATE_MS)) {
    		return RV_SUCCESS;
    	} else return RV_ERROR;
    }
    return RV_SUCCESS;
}

retval_t lithium_push_incoming_command_response(const frame_t *frame) {
	frame_t *dropped;
    if (pdFALSE == xQueueSendToBack(LITHIUM_STATE.queue_cmd_response, &frame, 0)) {
    	(void)xQueueReceive(LITHIUM_STATE.queue_cmd_response, &dropped, 0);
        log_report(LOG_RADIO, "Dropping a frame from queue_cmd_response\n");
    	frame_dispose(dropped);
    	if (pdTRUE == xQueueSendToBack(LITHIUM_STATE.queue_cmd_response, &frame, 1000 / portTICK_RATE_MS)) {
    		return RV_SUCCESS;
    	} else return RV_ERROR;
    }
    return RV_SUCCESS;
}

retval_t lithium_recv_data(frame_t **pFrame) {
    signed portBASE_TYPE rv;
    rv = xQueueReceive(LITHIUM_STATE.queue_data, pFrame, LITHIUM_CMD_TIMEOUT_TICKS);
    if (rv == pdTRUE) return RV_SUCCESS;
    else return RV_TIMEOUT;
}

retval_t lithium_recv_cmd_response(frame_t **pFrame) {
    signed portBASE_TYPE rv;
    *pFrame = NULL;
    rv = xQueueReceive(LITHIUM_STATE.queue_cmd_response, pFrame, LITHIUM_CMD_TIMEOUT_TICKS);
    if (rv == pdTRUE) return RV_SUCCESS;
    else return RV_TIMEOUT;
}

static inline
retval_t lithium_recv_cmd_expect(frame_t **pFrame, lithium_cmd_t cmd_code) {
    retval_t rv;
	lithium_cmd_t cmd = ~cmd_code; /* initialize to something different than cmd_code because the while code uses it */
	uint16_t payload_size;

    do {
        rv = lithium_recv_cmd_response(pFrame);
        if (RV_SUCCESS != rv) {
            log_report_fmt(LOG_RADIO, "lithium_recv_cmd_expect() timeout waiting for 0x%02x/%s\n", cmd_code, lithium_cmd_s(cmd_code));
            return rv;
        }
        rv = frame_advance(*pFrame, -(int)(sizeof(uint8_t)+sizeof(payload_size)+CHECKSUM_SIZE));
        if (RV_SUCCESS != rv) {
             frame_dispose(*pFrame);
             continue;
        }
        cmd = frame_get_u8_nocheck(*pFrame);
        if (cmd != cmd_code) {
            log_report_fmt(LOG_RADIO_VERBOSE, "lithium_recv_cmd_expect() popped cmd 0x%02x/%s (expected 0x%02x/%s)\n", cmd, lithium_cmd_s(cmd), cmd_code, lithium_cmd_s(cmd_code));
        	frame_dispose(*pFrame);
        }
    } while (cmd != cmd_code);
    return RV_SUCCESS;
}

static inline
portBASE_TYPE lithium_send_cmd(frame_t *frame) {
    return xQueueSendToBack(LITHIUM_STATE.queue_tx, &frame, portMAX_DELAY);
    /* FIXME retval */
}

void lithium_pop_outgoing(frame_t **pFrame) {
    (void)xQueueReceive(LITHIUM_STATE.queue_tx, pFrame, portMAX_DELAY);
}

retval_t lithium_basic_init_and_check() {
    retval_t rv;
    int telemetry_op_counter, tries;
    lithium_telemetry_t telemetry;

    rv = lithium_reset();
    log_report_fmt(LOG_RADIO_VERBOSE, "(init) lithium_reset: %s\n", retval_s(rv));
    vTaskDelay(2000 / portTICK_RATE_MS);

    tries = 0;
    telemetry_op_counter = 0;
    while (tries < 5) {
        tries++;

        /* get telemetry a few times until the op counter increments.
         * Just getting telemetry increments the counter */

        rv = lithium_get_telemetry(&telemetry);
        log_report_fmt(LOG_RADIO_VERBOSE, "(init) lithium_get_telemetry#%d opcounter %d: %s\n", tries, telemetry.op_counter, retval_s(rv));
        if (rv != RV_SUCCESS) continue;

        if (0 == telemetry_op_counter) {
        	telemetry_op_counter = telemetry.op_counter;
        } else {
			if (telemetry.op_counter > telemetry_op_counter) {
				return RV_SUCCESS;
			}
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    return RV_ERROR;
}

static retval_t lithium_transact(frame_t *send_frame, uint32_t delay_ms, frame_t **recv_frame) {
    frame_t *response;
    uint8_t command;
	retval_t rv;

	if (!frame_hasEnoughData(send_frame, MINIMUM_FRAME_SIZE)) return RV_NOSPACE;
	frame_advance(send_frame, 3);
	command = get_li1cmd_from_frame_nocheck(send_frame);
	frame_reset(send_frame);

	xSemaphoreTake(LITHIUM_STATE.mutex_command_send, portMAX_DELAY);
    lithium_send_cmd(send_frame);

    rv = lithium_recv_cmd_expect(&response, command);
    if (RV_SUCCESS == rv) {
        if (recv_frame == NULL) {
            frame_reset(response);
            frame_advance(response, 4);
            rv = LITHIUM_IS_NACK(frame_get_u16_nocheck(response)) ? RV_NACK : RV_SUCCESS;
            frame_dispose(response);
        }
        else {
            *recv_frame = response;
        }
    }
    xSemaphoreGive(LITHIUM_STATE.mutex_command_send);
    return rv;
}

static
retval_t lithium_send_frame(lithium_cmd_t command, frame_t *command_payload_or_NULL, frame_t **pResponse)
{
    retval_t rv;
    frame_t *command_frame;

    if(command_payload_or_NULL != NULL && frame_hasEnoughData(command_payload_or_NULL, FRAME_PAYLOAD_MAXSIZE+1)) {
        frame_dispose(command_payload_or_NULL);
        return RV_NOSPACE;
    }

    rv = frame_allocate(&command_frame);
    if (RV_SUCCESS != rv) {
    	if (command_payload_or_NULL != NULL) frame_dispose(command_payload_or_NULL);
    	return rv;
    }

    rv = create_command_frame(command, command_payload_or_NULL, command_frame);
    if (command_payload_or_NULL != NULL) frame_dispose(command_payload_or_NULL);

    if (rv != RV_SUCCESS) {
    	frame_dispose(command_frame);
    	return rv;
    }

    /* `create_command_frame()` resets and puts data into `frame`, at this point `position` will be the data size */
    frame_reset_for_reading(command_frame);
    log_report_fmt(LOG_RADIO_VERBOSE, "lithium_send_frame: pushed cmd 0x%02x (framelen: %d) to TX queue\n", command, command_frame->size);

    return lithium_transact(command_frame, 0, pResponse);
}

retval_t lithium_get_configuration(lithium_configuration_t * const config)
{
    frame_t *response;
    retval_t rv;
    uint16_t payload_size;

    rv = lithium_send_frame(LITHIUM_CMD_GET_TRANSCEIVER_CONFIG, NULL, &response);

    if (rv != RV_SUCCESS) return rv;

    payload_size = frame_get_u16_nocheck(response);
    if (LITHIUM_IS_NACK(payload_size)) {
    	frame_dispose(response);
    	return RV_ERROR;
    }

    frame_advance(response, CHECKSUM_SIZE); /* skip the checksum bytes */

    config->interface_baud_rate = frame_get_u8_nocheck(response);
    config->tx_power_amp_level  = frame_get_u8_nocheck(response);
    config->rx_rf_baud_rate     = frame_get_u8_nocheck(response);
    config->tx_rf_baud_rate     = frame_get_u8_nocheck(response);
    config->rx_modulation       = frame_get_u8_nocheck(response);
    config->tx_modulation       = frame_get_u8_nocheck(response);
    config->rx_freq             = frame_get_u32_le_nocheck(response);
    config->tx_freq             = frame_get_u32_le_nocheck(response);
    frame_get_data_nocheck(response, config->source, 6);
    frame_get_data_nocheck(response, config->destination, 6);
    config->tx_preamble         = frame_get_u16_le_nocheck(response);
    config->tx_postamble        = frame_get_u16_le_nocheck(response);
    config->function_config     = frame_get_u16_le_nocheck(response);
    config->function_config2     = frame_get_u16_le_nocheck(response);

    frame_dispose(response);

    return RV_SUCCESS;
}

static
retval_t configuration_to_command_frame(const lithium_configuration_t * configuration, frame_t * frame)
{
    frame_put_u8(frame, configuration->interface_baud_rate);
    frame_put_u8(frame, configuration->tx_power_amp_level);
    frame_put_u8(frame, configuration->rx_rf_baud_rate);
    frame_put_u8(frame, configuration->tx_rf_baud_rate);
    frame_put_u8(frame, configuration->rx_modulation);
    frame_put_u8(frame, configuration->tx_modulation);

    frame_put_u32_le(frame, configuration->rx_freq);
    frame_put_u32_le(frame, configuration->tx_freq);

    frame_put_data(frame, configuration->source, sizeof(configuration->source));
    frame_put_data(frame, configuration->destination, sizeof(configuration->destination));

    frame_put_u16_le(frame, configuration->tx_preamble);
    frame_put_u16_le(frame, configuration->tx_postamble);
    frame_put_u16_le(frame, configuration->function_config);
    frame_put_u16_le(frame, configuration->function_config2);
    return RV_SUCCESS;
}

retval_t lithium_send_data(frame_t *data) {
	return lithium_send_frame(LITHIUM_CMD_TRANSMIT_DATA, data, NULL);
}

retval_t lithium_get_telemetry(lithium_telemetry_t *telemetry) {
    frame_t *response;
    retval_t rv;
    uint16_t payload_size;
    
    rv = lithium_send_frame(LITHIUM_CMD_TELEMETRY_QUERY, NULL, &response);

    if (rv != RV_SUCCESS) return rv;

    payload_size = frame_get_u16_nocheck(response);
    if (LITHIUM_IS_NACK(payload_size)) {
    	frame_dispose(response);
    	return RV_ERROR;
    }

    frame_advance(response, CHECKSUM_SIZE); /* skip the checksum bytes */

    telemetry->op_counter        = frame_get_u16_le_nocheck(response);
    telemetry->msp430_temp       = frame_get_u16_le_nocheck(response);
    telemetry->time_count[0]     = frame_get_u8_nocheck(response);
    telemetry->time_count[1]     = frame_get_u8_nocheck(response);
    telemetry->time_count[2]     = frame_get_u8_nocheck(response);
    telemetry->rssi              = frame_get_u8_nocheck(response);
    telemetry->bytes_received    = frame_get_u32_le_nocheck(response);
    telemetry->bytes_transmitted = frame_get_u32_le_nocheck(response);

    frame_dispose(response);

    return RV_SUCCESS;
}

retval_t lithium_set_configuration(const lithium_configuration_t *config)
{
    frame_t *cmd_data;
    retval_t rv;

    rv = frame_allocate(&cmd_data);
    if (RV_SUCCESS != rv) return rv;

    configuration_to_command_frame(config, cmd_data);
    frame_reset_for_reading(cmd_data);
    
    rv = lithium_send_frame(LITHIUM_CMD_SET_TRANSCEIVER_CONFIG, cmd_data, NULL);
	log_report_fmt(LOG_RADIO, "lithium_set_configuration: %d\n", retval_s(rv));
    
    return rv;
}

retval_t lithium_set_rf_configuration(const lithium_rf_configuration_t *rf_config) {
    frame_t *cmd_data;
    retval_t rv;

    rv = frame_allocate(&cmd_data);
    if (RV_SUCCESS != rv) return rv;

    frame_put_data(cmd_data, rf_config, sizeof(lithium_rf_configuration_t));	/* Proper serializing using _le */
    frame_reset_for_reading(cmd_data);
    
    rv = lithium_send_frame(LITHIUM_CMD_RF_CONFIG, cmd_data, NULL);
    
    return rv;
}

retval_t lithium_write_configuration_to_flash(void *md5_digest) {

    frame_t cmd_data;

    cmd_data.buf = md5_digest;
    cmd_data.size = MD5_DIGEST_SIZE;
    frame_reset(&cmd_data);

    return lithium_send_frame(LITHIUM_CMD_WRITE_FLASH, &cmd_data, NULL);
}

retval_t lithium_set_beacon_data(frame_t *data) {
	retval_t rv;
	frame_t data_copy;
	frame_copy(&data_copy, data);

	last_beacon_data_for_workaround.size = MAX_FRAME_SIZE;		// Manual frame recycle... frame needs a rework
	frame_transfer(&last_beacon_data_for_workaround, &data_copy);
	frame_reset_for_reading(&last_beacon_data_for_workaround);

	rv = lithium_send_frame(LITHIUM_CMD_BEACON_DATA, data, NULL);
	return rv;
}

retval_t lithium_set_beacon_interval(unsigned int interval_s) {
	retval_t rv;
    uint32_t interval = LITHIUM_BEACON_SECONDS_TO_VALUE(interval_s);
	frame_t data = DECLARE_FRAME_BYTES(interval <= 255 ? interval : 255);

	rv = lithium_send_frame(LITHIUM_CMD_BEACON_CONFIG, &data, NULL);
	return rv;
}

retval_t lithium_set_tx_power(unsigned int power) {
	retval_t rv;
	frame_t data = DECLARE_FRAME_BYTES(power);

	log_report_fmt(LOG_RADIO_VERBOSE, "lithium_set_tx_power(%d)\n", power);
	rv = lithium_send_frame(LITHIUM_CMD_FAST_PA_SET, &data, NULL);
	return rv;
}

retval_t lithium_set_rx_bps(uint8_t rx_bps) {
    /* fixme: implement it */
    /* WARNING: if radio RX data rate is set to a value the ground station can't send,
       we risk losing communication with the satellite. Just saying */
    return RV_NOTIMPLEMENTED;
}

retval_t lithium_set_config_bps(lithium_configuration_t *config, uint8_t tx_bps) {
	uint32_t frequency_KHz;
	uint8_t modulation;

	if (tx_bps > LITHIUM_RF_BAUD_RATE_38400) return RV_ILLEGAL;
    if (config->tx_rf_baud_rate == tx_bps) return RV_SUCCESS;

    frequency_KHz = config->tx_freq;

    if (config->tx_modulation == LITHIUM_RF_MODULATION_AFSK) {
    	frequency_KHz -= LITHIUM_AFSK_FREQUENCY_SHIFT_KHz;
    } else {
    	frequency_KHz -= LITHIUM_FSK_FREQUENCY_SHIFT_KHz;
    }

    /* if tx_bps == 1200, only allowed modulation is AFSK else, use GFSK. */
    if (tx_bps == LITHIUM_RF_BAUD_RATE_1200) {
        modulation = LITHIUM_RF_MODULATION_AFSK;
        frequency_KHz += LITHIUM_AFSK_FREQUENCY_SHIFT_KHz;
    }
    else {
        modulation = LITHIUM_RF_MODULATION_GFSK;
        frequency_KHz += LITHIUM_FSK_FREQUENCY_SHIFT_KHz;
    }

    config->tx_rf_baud_rate = tx_bps;
    config->tx_modulation = modulation;
    config->tx_freq = frequency_KHz;
    config->rx_freq = frequency_KHz;
    return RV_SUCCESS;
}

retval_t lithium_set_tx_bps(uint8_t tx_bps) {
    lithium_configuration_t config;
    retval_t rv;

    rv = lithium_get_configuration(&config);
    SUCCESS_OR_RETURN(rv);

    rv = lithium_set_config_bps(&config, tx_bps);
    SUCCESS_OR_RETURN(rv);

    return lithium_set_configuration(&config);
}

retval_t lithium_noop() {
	return lithium_send_frame(LITHIUM_CMD_NO_OP, NULL, NULL);
}

retval_t lithium_reset() {
	log_report(LOG_RADIO, "lithium_reset\n");
	return lithium_send_frame(LITHIUM_CMD_RESET_SYSTEM, NULL, NULL);
}

const char *
lithium_cmd_s(lithium_cmd_t cmd)
{
    static const char *li_cmd_n[] = {
        [LITHIUM_CMD_NO_OP] = "NOOP",
        [LITHIUM_CMD_RESET_SYSTEM] = "RESET",
        [LITHIUM_CMD_TRANSMIT_DATA] = "TRANSMIT",
        [LITHIUM_CMD_RECEIVE_DATA] = "RECEIVE",
        [LITHIUM_CMD_GET_TRANSCEIVER_CONFIG] = "GET_CFG",
        [LITHIUM_CMD_SET_TRANSCEIVER_CONFIG] = "SET_CFG",
        [LITHIUM_CMD_TELEMETRY_QUERY] = "TELEMETRY",
        [LITHIUM_CMD_WRITE_FLASH] = "WRITE_FLASH",
        [LITHIUM_CMD_RF_CONFIG] = "RF_CONFIG",
        [LITHIUM_CMD_BEACON_DATA] = "BEACON_DAT",
        [LITHIUM_CMD_BEACON_CONFIG] = "BEACON_CFG",
        [LITHIUM_CMD_READ_FIRMWARE_REVISION] = "FW_REV",
        [LITHIUM_CMD_WRITE_OVER_AIR_KEY] = "AIR_KEY",
        [LITHIUM_CMD_FIRMWARE_UPDATE] = "FW_UPDATE",
        [LITHIUM_CMD_FIRMWARE_PACKET] = "FW_PKT",
        [LITHIUM_CMD_FAST_PA_SET] = "FAST_SET",
    };

    if (cmd >= ARRAY_COUNT(li_cmd_n) || NULL == li_cmd_n[cmd]) {
    	return "BUGGY";
    }

    return li_cmd_n[cmd];
}
