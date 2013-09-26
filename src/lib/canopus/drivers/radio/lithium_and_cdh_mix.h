#ifndef _LITHIUM_AND_CDH_MIX_H
#define _LITHIUM_AND_CDH_MIX_H

#include <canopus/types.h>
#include <canopus/frame.h>

#include <FreeRTOS.h>
#include <task.h>

struct LITHIUM_AND_CDH_MIX_STATE_st {
    xQueueHandle queue_data;
    xQueueHandle queue_tx;

    xQueueHandle queue_cmd_response;
    xSemaphoreHandle mutex_command_send;
};

extern struct LITHIUM_AND_CDH_MIX_STATE_st LITHIUM_STATE;

retval_t lithium_basic_init_and_check(void);

retval_t lithium_parse_command_frame(frame_t *frame, lithium_cmd_t *command, bool *is_ack);

retval_t lithium_recv_cmd_response(frame_t **pFrame);

retval_t lithium_push_incoming_data(const frame_t *frame);
retval_t lithium_push_incoming_command_response(const frame_t *frame);

void lithium_pop_outgoing(frame_t **pFrame);

#endif
