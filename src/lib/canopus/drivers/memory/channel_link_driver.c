/* a channel link that lives inside a buffer - analogous to python's stringio
 * INCOMPLETE IMPLEMENTATION - ONLY TO BE USED FOR TESTING - NOT INTENDED FOR PRODUCTION
 *
 *
 * I SAY IT AGAIN: THIS IMPL FSCKING SUCKS. DON'T USE IT.
 *
 *
 * auteur: manuel
 */
#include <string.h>

#include <canopus/drivers/memory/channel_link_driver.h>
#include <canopus/frame.h>
#include <canopus/assert.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

static retval_t _open(const channel_t * const link)
{

    if (link         == NULL) return RV_ILLEGAL;
    if (link->config == NULL) return RV_ILLEGAL;
    if (link->state  == NULL) return RV_ILLEGAL;

    const memory_channel_config_t * const channel_config =
        (const memory_channel_config_t *) link->config;
    memory_channel_state_t * channel_state =
        (memory_channel_state_t *) link->state;

    if (channel_config->recv_frame == NULL) return RV_ILLEGAL;
    if (channel_config->recv_frame->size == 0) return RV_ILLEGAL;
    if (channel_config->send_frame == NULL) return RV_ILLEGAL;
    if (channel_config->send_frame->size == 0) return RV_ILLEGAL;

    channel_state->recv_frame = channel_config->recv_frame;
    channel_state->send_frame = channel_config->send_frame;
    frame_reset(channel_state->recv_frame);
    frame_reset(channel_state->send_frame);
    channel_state->is_open = true;
    channel_state->send_mutex = xSemaphoreCreateMutex();
    channel_state->recv_mutex = xSemaphoreCreateMutex();
    channel_state->available_bytes = 0;

    return RV_SUCCESS;
}

static retval_t _close(const channel_t * const link)
{
    if (link         == NULL) return RV_ILLEGAL;
    if (link->state  == NULL) return RV_ILLEGAL;

    memory_channel_state_t * const channel_state =
        (memory_channel_state_t *) link->state;

    if (channel_state->is_open)
        channel_state->is_open = false;

    return RV_SUCCESS;
}

static retval_t _send(
		const channel_t * const link,
		frame_t * const send_frame,
		const size_t count)
{
    memory_channel_state_t * channel_state =
        (memory_channel_state_t *) link->state;

    retval_t rv;
    char *dp;

	assert(link);
	assert(link->state);
	assert(send_frame);

    if (!channel_state->is_open) return RV_ILLEGAL;
    if (0 == count) return RV_SUCCESS;

    xSemaphoreTake(channel_state->send_mutex, portMAX_DELAY);

    if (!frame_hasEnoughData(send_frame, count)) {
        xSemaphoreGive(channel_state->send_mutex);
    	return RV_NOSPACE;
    }

    dp = frame_get_data_pointer_nocheck(send_frame, count);
    rv = frame_put_data(channel_state->send_frame, dp, count);

    xSemaphoreGive(channel_state->send_mutex);
    return rv;
}

static retval_t _recv(
    const channel_t * const link,
    frame_t * const recv_frame,
    const size_t count)
{
    memory_channel_state_t * channel_state =
        (memory_channel_state_t *) link->state;

    int n;
    retval_t rv;
    char *buf;

	assert(link);
	assert(link->state);
	assert(recv_frame);

    if (!channel_state->is_open) return RV_ILLEGAL;
    if (0 == count) return RV_SUCCESS;

    while (channel_state->available_bytes < 1) vTaskDelay(100);

    xSemaphoreTake(channel_state->recv_mutex, portMAX_DELAY);

    /* TODO check retvalues and stuff */

    n = count > channel_state->available_bytes ? channel_state->available_bytes : count;

    buf = (char *) channel_state->recv_frame->buf;
    rv = frame_put_data(recv_frame, buf + channel_state->read_pos, n);

    if (rv == RV_SUCCESS) {
    	channel_state->read_pos += n;
    	channel_state->available_bytes -= n;
    }

    xSemaphoreGive(channel_state->recv_mutex);

    return rv;
}

retval_t memory_channel_write_for_recv(const channel_t *const link, frame_t *frame) {

    /* this function lacks every check possible.
     * it's not meant for production use, anyway */
	retval_t rv;
	size_t count;
	void *buf;

    memory_channel_state_t * channel_state =
        (memory_channel_state_t *) link->state;

    xSemaphoreTake(channel_state->recv_mutex, portMAX_DELAY);

    if (0 == channel_state->available_bytes) {
    	frame_reset(channel_state->recv_frame);
    	channel_state->read_pos = 0;
    }
    count = _frame_available_data(frame);
    buf = frame_get_data_pointer_nocheck(frame, count);
    rv = frame_put_data(channel_state->recv_frame, buf, count);

    channel_state->available_bytes += count;

    xSemaphoreGive(channel_state->recv_mutex);

    return rv;
}

frame_t *memory_channel_get_send_frame(const channel_t * const link) {
    memory_channel_state_t * channel_state =
        (memory_channel_state_t *) link->state;

    return channel_state->send_frame;
}

static retval_t _transact(
    const channel_t * const link,
    frame_t * const send_frame,
    uint32_t delay_ms,
    frame_t * const recv_frame,
    const size_t send_bytes,
    const size_t recv_bytes)
{
    retval_t rv;
    if (recv_frame == NULL && send_frame == NULL)
        return RV_ILLEGAL;

    if (send_frame != NULL) {
        rv = _send(link, send_frame, send_bytes);
        if (rv != RV_SUCCESS)
            return rv;
    }
    vTaskDelay(delay_ms / portTICK_RATE_MS);
    if (recv_frame != NULL) {
        rv = _recv(link, recv_frame, recv_bytes);
        if (rv != RV_SUCCESS)
            return rv;
    }
    return RV_SUCCESS;
}

static retval_t _initialize(
    const channel_driver_t        * const driver)
{
    return RV_SUCCESS;
}

static retval_t _deinitialize(
    const channel_driver_t * const driver)
{
    return RV_SUCCESS;
}

static const channel_driver_api_t
memory_channel_driver_api = {
    .initialize   = &_initialize,
    .deinitialize = &_deinitialize,
    .open     = &_open,
    .close    = &_close,
    .send     = &_send,
    .recv     = &_recv,
    .transact = &_transact
};

const channel_driver_t memory_channel_driver = DECLARE_CHANNEL_DRIVER(&memory_channel_driver_api, NULL, channel_driver_state_t);
