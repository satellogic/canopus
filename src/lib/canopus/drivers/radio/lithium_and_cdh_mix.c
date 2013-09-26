/*
   Astrodev Li-1 API implementation
   auteur: manuel
*/
#include <canopus/assert.h>
#include <canopus/frame.h>
#include <canopus/md5.h>
#include <canopus/logging.h>
#include <canopus/board/channels.h>
#include <canopus/drivers/radio/lithium.h>
#include <canopus/subsystem/cdh.h> /* CDH_process_hard_commands */

#include <FreeRTOS.h>
#include <task.h>

#include <string.h>

#include "lithium_and_cdh_mix.h"

#define LITHIUM_RX_TASK_PRIORITY ( tskIDLE_PRIORITY + 2 ) /* TODO define this */
#define LITHIUM_TX_TASK_PRIORITY ( tskIDLE_PRIORITY + 2 ) /* TODO define this */

/*
#define LITHIUM_QUEUE_DATA_SIZE 16
#define LITHIUM_QUEUE_COMMAND_ACK_SIZE 16
*/
#define LITHIUM_QUEUE_TX_SIZE	16

/*
   THIS DRIVER CONSIDERS A SINGLE "STATIC" LITHIUM
   STATE IS KEPT IN THE STATIC STRUCT `LITHIUM_STATE`
*/

/* Lithium state struct */
static struct LITHIUM_CDH_STATE_st {
    xTaskHandle radio_tx_task_handle;
    xTaskHandle radio_rx_task_handle;
    xTaskHandle umbilical_rx_task_handle;
} STATE = { };

struct LITHIUM_AND_CDH_MIX_STATE_st LITHIUM_STATE = { };

static
retval_t advance_to_next_lithium_header(frame_t * frame) {
    assert(frame != NULL);

    while (frame_hasEnoughData(frame, 1)) {
        if (frame_get_u8_nocheck(frame) == LITHIUM_SYNC_CHAR_1) {
                frame_advance(frame, -1);
                return RV_SUCCESS;
        }
    }
    return RV_NOSPACE;
}

static
retval_t process_lithium_packet(frame_t *frame, lithium_cmd_t command, bool is_ack)
{
    retval_t rv;

    switch(command) {
    case LITHIUM_CMD_RECEIVE_DATA:
        frame->size -= 2;	/* remove trailing CKSUM */ // FIXME is it AX25_TRAILER?

        rv = advance_over_ax25(frame);
        if (RV_SUCCESS != rv) {
        	frame_dispose(frame);
        	return rv;
        }

        rv = CDH_process_hard_commands(frame);
        if (RV_SUCCESS == rv)
        	break;

        rv = lithium_push_incoming_data(frame);
        if (RV_SUCCESS != rv) {
        	frame_dispose(frame);
            return RV_ERROR;
        }
        break;
    default:
    	if (pdTRUE != xSemaphoreTake(LITHIUM_STATE.mutex_command_send, 0)) {
    		/* If we can't get the semaphore it means somebody is waiting for a response */
			rv = lithium_push_incoming_command_response(frame);
			if (RV_SUCCESS != rv) {
				/* should not happen: because queue_cmd_response size == pool size */
				frame_dispose(frame);
				return RV_ERROR;
			}
    	} else {
    		xSemaphoreGive(LITHIUM_STATE.mutex_command_send);
            log_report_fmt(LOG_RADIO_VERBOSE, "LITHIUM: Just dropped a response for cmd 0x%02x (this is good)\n", command);
			frame_dispose(frame);
		}
		break;
    }
    return RV_SUCCESS;
}

static
void lithium_rx_task(void *pvParameters) {
	uint8_t emergency_frame_buffer[300];
	frame_t emergency_frame = DECLARE_FRAME(emergency_frame_buffer);
    channel_t *ch_input = (channel_t *)pvParameters;
    retval_t rv;
    frame_t *frame, *next_frame;
    frame_t hdr_frame, payload_frame;
    lithium_cmd_t cmd;
    bool is_ack, do_read;

    rv = frame_allocate_retry(&frame, 3000 / portTICK_RATE_MS);
    if (RV_SUCCESS != rv) {
    	log_report(LOG_RADIO, "No frame?!\n");
    	frame = &emergency_frame;
    }
    do_read = true;

    for (;;) {
#ifdef DEBUG_FRAMES
    	log_report_fmt(LOG_RADIO, "queue_data: %d\n   queue_tx: %d\n   queue_cmd_response: %d\n   free frames: %d\n",
        		uxQueueMessagesWaiting(LITHIUM_STATE.queue_data),
        		uxQueueMessagesWaiting(LITHIUM_STATE.queue_tx),
				uxQueueMessagesWaiting(LITHIUM_STATE.queue_cmd_response),
				frame_free_count());
#endif
        if (do_read) {
        	if (!frame_hasEnoughSpace(frame, 1)) {
            	frame_recycle(frame);
        	}
            rv = channel_recv(ch_input, frame);
            if (RV_TIMEOUT == rv) continue;
            /* FIXME if rv != RV_SUCCESS */
#ifdef WANT_MORE_VERBOSITY
            size_t read_bytes;
            read_bytes = frame->position;
            if (read_bytes) {
            	log_report_fmt(LOG_RADIO, "lithium_rx_task: got %d bytes from channel\n", read_bytes);
            }
#endif
        }
        do_read = true;

        frame_copy_for_reading(&hdr_frame, frame);
        if (!frame_hasEnoughData(&hdr_frame, MINIMUM_FRAME_SIZE)) continue;

        while (RV_SUCCESS == advance_to_next_lithium_header(&hdr_frame)) {
        	/* here hdr_frame points to 'He' */
            /* make payload_frame point to 'He' */
            frame_copy(&payload_frame, &hdr_frame);

            rv = lithium_parse_command_frame(&hdr_frame, &cmd, &is_ack);
            /* if RV_SUCCESS hdr_frame points to the byte after all Li-1 packet
             * Otherwise hdr_frame is not used anymore */

            switch(rv) {
            case RV_SUCCESS:
            	/* make payload frame point to Li-1 payload */
                frame_advance(&payload_frame, FRAME_HEADER_SIZE);

                /* Truncate payload frame to the payload */
                payload_frame.size = hdr_frame.position;

                /* current frame will be enqueued, get another one */
                rv = frame_allocate_retry(&next_frame, 1500 / portTICK_RATE_MS);
                if (RV_SUCCESS != rv) {
                    /* If no more frames, drop the oldest command response, or incomming data */
                	rv = lithium_recv_cmd_response(&next_frame);
                	if (RV_SUCCESS != rv) {
                		rv = lithium_recv_data(&next_frame);
                		if (RV_SUCCESS != rv) {
                            log_report(LOG_RADIO, "Out of frames :(\n");

                            next_frame = &emergency_frame;
                            emergency_frame.size = sizeof(emergency_frame_buffer);
                            frame_reset(next_frame);
                		} else frame_recycle(next_frame);
                	} else frame_recycle(next_frame);
                }

                if (RV_SUCCESS == advance_to_next_lithium_header(&hdr_frame)) {
                	/* There is an 'H' in the frame after the received packet */
					/* Pass this possible Li-1 frame in the tail of hdr_frame, to next_frame */
					frame_transfer(next_frame, &hdr_frame);
					do_read = false;
                }

                /* We need to copy it, so the right frame is disposed */
                frame_copy(frame, &payload_frame);
                process_lithium_packet(frame, cmd, is_ack); // XXX retval?

                frame = next_frame;
                break;
                /* Any other case MUST advance the hdr_frame, otherwise it'll fall into an infinite loop
                 * (and the current hdr has been already discarded for the current pass)
                 */
            default:
                /* FIXME: report abnormal situation */
            case RV_ILLEGAL:
            case RV_NOSPACE:
                frame_copy(&hdr_frame, &payload_frame);
                frame_advance(&hdr_frame, 1);
                break;
            }
        }
    }
}

static
void lithium_tx_task(void *pvParameters) {
	channel_t *ch_output = (channel_t*)pvParameters;
    frame_t * frame;
    retval_t rv;
    size_t last_available;

    log_report(LOG_RADIO, "TASK: lithium_tx_task started\n");
    while (1) {
        lithium_pop_outgoing(&frame); /* blocks here */
        log_report_fmt(LOG_RADIO, "LITHIUM: tx - got a frame to send (%d) bytes\n", _frame_available_data(frame));

        /* ToDo: Loop until all data is sent.
         * a few tries? while no RV_TIMEOUT?
         */
        while (1) {
        	last_available = _frame_available_data(frame);
        	rv = channel_send(ch_output, frame);
        	if (last_available == _frame_available_data(frame)) break;
        	if (RV_PARTIAL != rv) break;
        }

        /* replicate the same output in the console/umbilical */
        frame_reset(frame);
        (void)channel_send(ch_umbilical_out, frame);

        frame_dispose(frame);
    }
}

/* lithium API implementation */
retval_t lithium_initialize(const channel_t *radio_channel) {
    retval_t rv = RV_ERROR;
    portBASE_TYPE xReturn;

    assert(radio_channel != NULL);

    /* queues, mutexes and tasks */

    LITHIUM_STATE.queue_data = xQueueCreate(frame_free_count(), sizeof(frame_t *));
    if (NULL == LITHIUM_STATE.queue_data) {
        return RV_NOSPACE;
    }

    LITHIUM_STATE.queue_tx = xQueueCreate(LITHIUM_QUEUE_TX_SIZE, sizeof(frame_t *));
    if (NULL == LITHIUM_STATE.queue_tx) {
        return RV_NOSPACE;
    }

    LITHIUM_STATE.queue_cmd_response = xQueueCreate(frame_free_count(), sizeof(frame_t *));
    if (NULL == LITHIUM_STATE.queue_cmd_response) {
        return RV_NOSPACE;
    }

    LITHIUM_STATE.mutex_command_send = xSemaphoreCreateMutex();
    if (NULL == LITHIUM_STATE.mutex_command_send) {
        return RV_NOSPACE;
    }

    xReturn = xTaskCreate(
            &lithium_tx_task,
            (signed char *)"LITHIUM/TX",
            configMINIMAL_STACK_SIZE + 1024,
            (void*)radio_channel,
            LITHIUM_TX_TASK_PRIORITY,
            &STATE.radio_tx_task_handle);
    /* we want at least some output... */
    assert(pdTRUE == xReturn);

    if (pdTRUE == xTaskCreate(
    		&lithium_rx_task,
            (signed char *)"LITHIUM/RX Radio",
            configMINIMAL_STACK_SIZE + 1024,
    		(void*)radio_channel,
    		LITHIUM_RX_TASK_PRIORITY,
    		&STATE.radio_rx_task_handle)) {

        rv = lithium_basic_init_and_check();
    }

    (void /* If fails in flight we don't care */)xTaskCreate(
            &lithium_rx_task,
            (signed char *) "LITHIUM/RX Umbilical",
            configMINIMAL_STACK_SIZE + 1024,
            (void*)ch_umbilical_in,
            LITHIUM_RX_TASK_PRIORITY,
            &STATE.umbilical_rx_task_handle);

    return rv;
}

#ifdef CMOCKERY_TESTING
void lithium_deinitialize() {
    vTaskDelete(STATE.umbilical_rx_task_handle);
    vTaskDelete(STATE.radio_rx_task_handle);
    vTaskDelete(STATE.radio_tx_task_handle);
    vSemaphoreDelete(LITHIUM_STATE.mutex_command_send);
    vQueueDelete(LITHIUM_STATE.queue_data);
    vQueueDelete(LITHIUM_STATE.queue_tx);
    vQueueDelete(LITHIUM_STATE.queue_cmd_response);
}
#endif /* CMOCKERY_TESTING */
