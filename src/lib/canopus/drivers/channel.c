#include <canopus/assert.h>
#include <canopus/drivers/channel.h>
#include <FreeRTOS.h>
#include <task.h>

static retval_t _channel_state_send_lock(const channel_state_t *c_state, channel_config_t const* c_config, portTickType xBlockTime_ms) {
	if (false == c_state->is_open) return RV_ILLEGAL;
	if (NULL == c_state->send_mutex) return RV_NACK;

	if ((c_config->flags & CHANNEL_FLAG_NO_AUTO_LOCK) == CHANNEL_FLAG_NO_AUTO_LOCK) {
		return RV_SUCCESS;
	}

	if (0 == xBlockTime_ms) xBlockTime_ms = c_config->lock_timeout_ms;

    if (pdFALSE == xSemaphoreTakeRecursive(c_state->send_mutex, xBlockTime_ms / portTICK_RATE_MS)) {
        return RV_TIMEOUT;
    }
    return RV_SUCCESS;
}

static retval_t _channel_state_recv_lock(const channel_state_t *c_state, const channel_config_t *c_config, portTickType xBlockTime_ms) {
	if (false == c_state->is_open) return RV_ILLEGAL;
	if (NULL == c_state->recv_mutex) return RV_NACK;

	if ((c_config->flags & CHANNEL_FLAG_NO_AUTO_LOCK) == CHANNEL_FLAG_NO_AUTO_LOCK) {
		return RV_SUCCESS;
	}

	if (0 == xBlockTime_ms) xBlockTime_ms = c_config->lock_timeout_ms;

    if (pdFALSE == xSemaphoreTakeRecursive(c_state->recv_mutex, xBlockTime_ms / portTICK_RATE_MS)) {
        return RV_TIMEOUT;
    }
    return RV_SUCCESS;
}

static retval_t _channel_state_send_unlock(const channel_state_t *c_state) {
	if (false == c_state->is_open) return RV_ILLEGAL;
	if (NULL == c_state->send_mutex) return RV_NACK;

    xSemaphoreGiveRecursive(c_state->send_mutex);
    return RV_SUCCESS;
}

static retval_t _channel_state_recv_unlock(const channel_state_t *c_state) {
	if (false == c_state->is_open) return RV_ILLEGAL;
	if (NULL == c_state->recv_mutex) return RV_NACK;

    xSemaphoreGiveRecursive(c_state->recv_mutex);
    return RV_SUCCESS;
}

retval_t channel_open(const channel_t *const channel)
{
	channel_state_t *c_state;
	channel_driver_t const *driver;
	channel_open_t *_open;
	retval_t rv;

	assert(channel);
	c_state = channel->state;
	driver  = channel->driver;

	assert(c_state);
	assert(channel->config);
	assert(driver);
	assert(driver->api);
	assert(driver->state);

	if (true != driver->state->is_initialized) return RV_ILLEGAL;
	if (true == c_state->is_open) return RV_ILLEGAL;

	if (NULL == c_state->send_mutex) {
		// Allow reopening after close, reuse mutex

		c_state->send_mutex = xSemaphoreCreateRecursiveMutex();
		if (NULL == c_state->send_mutex) {
			return RV_NOSPACE;
		}
	}

	if (NULL == c_state->recv_mutex) {
		// Allow reopening after close, reuse mutex

		c_state->recv_mutex = xSemaphoreCreateRecursiveMutex();
		if (NULL == c_state->recv_mutex) {
			return RV_NOSPACE;
		}
	}

	rv = RV_SUCCESS;

	_open = driver->api->open;
	if (IS_PTR_VALID(_open)) {
		rv = _open(channel);
	}

	if (RV_SUCCESS == rv) {
		c_state->is_open = true;
	}

	return rv;
}

retval_t channel_close(const channel_t *const channel)
{
	channel_state_t *c_state;
	channel_close_t *_close;
	retval_t rv;

	assert(channel);
	c_state = channel->state;
	assert(c_state);
	assert(channel->config);

	if (false == c_state->is_open) return RV_ILLEGAL;

	// Here only if is_open. Only possible if driver and driver->api are not NULL and
	// If driver is_initialized.
	// If driver or driver->api are corrupted... why should they be corrupted with NULL?

	rv = RV_SUCCESS;

	_close = channel->driver->api->close;
	if (IS_PTR_VALID(_close)) {
		rv = _close(channel);
	}

	if (RV_SUCCESS == rv) {
		_channel_state_send_unlock(c_state);
		_channel_state_recv_unlock(c_state);
		c_state->is_open = false;
	}

	return rv;
}

retval_t channel_lock(const channel_t *const channel, portTickType xBlockTime_ms) {
	channel_state_t *c_state;
	retval_t send_rv, recv_rv;

	assert(channel);
	c_state = channel->state;
	assert(c_state);

	send_rv = _channel_state_send_lock(c_state, channel->config, xBlockTime_ms);

	if (RV_SUCCESS == send_rv) {
		recv_rv = _channel_state_recv_lock(c_state, channel->config, xBlockTime_ms);
		if (RV_SUCCESS != recv_rv) {
			_channel_state_send_unlock(c_state);
			send_rv = recv_rv;
		}
	}
	return send_rv;
}

retval_t channel_unlock(const channel_t *const channel) {
	channel_state_t *c_state;
	retval_t send_rv, recv_rv;

	assert(channel);
	c_state = channel->state;
	assert(c_state);

	if (false == c_state->is_open) return RV_ILLEGAL;

	send_rv = _channel_state_send_unlock(c_state);
	recv_rv = _channel_state_recv_unlock(c_state);

	if (RV_SUCCESS != send_rv) return send_rv;
	return recv_rv;
}

retval_t channel_send(const channel_t *const channel,
                      frame_t *const send_frame)
{
	channel_state_t *c_state;
	channel_send_t *_send;
	channel_transact_t *_transact;
	retval_t rv;

	assert(channel);
	c_state = channel->state;
	assert(c_state);
	assert(channel->config);

	if (NULL  == send_frame) return RV_ILLEGAL;
	if (!_frame_available_data(send_frame)) return RV_SUCCESS;

	// Here only if is_open. Only possible if driver and driver->api are not NULL and
	// If driver is_initialized.
	// If driver or driver->api are corrupted... why should they be corrupted with NULL?

	rv = _channel_state_send_lock(c_state, channel->config, 0);
	if ((RV_SUCCESS != rv) && (RV_NACK != rv)) return rv;

	rv = RV_SUCCESS;

	_send = channel->driver->api->send;
	if (IS_PTR_VALID(_send)) {
		rv = _send(channel, send_frame, _frame_available_data(send_frame));
	} else {
		_transact = channel->driver->api->transact;
		if (IS_PTR_VALID(_transact)) {
			rv = _transact(
			        channel, send_frame, 0, NULL,
			        _frame_available_data(send_frame), 0);
		}
	}

	_channel_state_send_unlock(c_state);
	return rv;
}

retval_t channel_recv(const channel_t *const channel,
                      frame_t *const recv_frame)
{
	channel_state_t *c_state;
	channel_recv_t *_recv;
	channel_transact_t *_transact;
	retval_t rv;

	assert(channel);
	c_state = channel->state;
	assert(c_state);
	assert(channel->config);

	if (NULL  == recv_frame) return RV_ILLEGAL;
	if (!_frame_available_space(recv_frame)) return RV_SUCCESS; // FIXME or RV_NOSPACE?

	// Here only if is_open. Only possible if driver and driver->api are not NULL and
	// If driver is_initialized.
	// If driver or driver->api are corrupted... why should they be corrupted with NULL?

	rv = _channel_state_recv_lock(c_state, channel->config, 0);
	if ((RV_SUCCESS != rv) && (RV_NACK != rv)) return rv;

	rv = RV_SUCCESS;

	_recv = channel->driver->api->recv;
	if (IS_PTR_VALID(_recv)) {
		rv = _recv(
		        channel, recv_frame, _frame_available_space(recv_frame));
	} else {
		_transact = channel->driver->api->transact;
		if (IS_PTR_VALID(_transact)) {
			rv = _transact(
			        channel, NULL, 0, recv_frame,
			        0, _frame_available_space(recv_frame));
		}
	}

	_channel_state_recv_unlock(c_state);
	return rv;
}

retval_t channel_transact(const channel_t *const channel, frame_t *const send_frame,
        uint32_t delay_ms, frame_t *const recv_frame)
{
	channel_state_t *c_state;
	channel_send_t *_send;
	channel_recv_t *_recv;
	channel_transact_t *_transact;
	size_t send_count, recv_count;
	retval_t rv;

	assert(channel);
	c_state = channel->state;
	assert(c_state);
	assert(channel->config);

	if (false == c_state->is_open) return RV_ILLEGAL;
	if ((NULL  == send_frame) && (NULL == recv_frame)) return RV_ILLEGAL;

	// Here only if is_open. Only possible if driver and driver->api are not NULL and
	// If driver is_initialized.
	// If driver or driver->api are corrupted... why should they be corrupted with NULL?

    if (NULL != send_frame) send_count = _frame_available_data(send_frame);
    else send_count = 0;

    if (NULL != recv_frame) recv_count = _frame_available_space(recv_frame);
    else recv_count = 0;

    if (send_count > 0) {
    	rv = _channel_state_send_lock(c_state, channel->config, 0);
    	if ((RV_SUCCESS != rv) && (RV_NACK != rv)) return rv;
    }

    if (recv_count > 0) {
    	rv = _channel_state_recv_lock(c_state, channel->config, 0);
    	if ((RV_SUCCESS != rv) && (RV_NACK != rv)) {
    		if (send_count > 0) _channel_state_send_unlock(c_state);
    		return rv;
    	}
    }

	rv = RV_SUCCESS;

	_transact = channel->driver->api->transact;
	if (IS_PTR_VALID(_transact)) {
		rv = _transact(
		        channel, send_frame, delay_ms, recv_frame,
		        send_count, recv_count);
	} else {
		_send = channel->driver->api->send;
		_recv = channel->driver->api->recv;
		if (IS_PTR_VALID(_send) && IS_PTR_VALID(_recv)) {
			if (send_count > 0) {
				rv = _send(channel, send_frame, send_count);
			}
			if (RV_SUCCESS == rv) {
				if (recv_count > 0) {
					vTaskDelay(delay_ms / portTICK_RATE_MS);
					rv = _recv(channel, recv_frame, recv_count);
				}
			}
		} else if (IS_PTR_VALID(_send) || IS_PTR_VALID(_recv)) {
			rv = RV_ILLEGAL;
		}
	}

	if (send_count > 0) _channel_state_send_unlock(c_state);
	if (recv_count > 0) _channel_state_recv_unlock(c_state);
	return rv;
}

retval_t channel_driver_initialize(
        const channel_driver_t        * const driver)
{
	retval_t rv;

    assert(driver);
    assert(driver->api);
    assert(driver->state);

    if (driver->state->is_initialized) return RV_ILLEGAL;

    if ((!IS_PTR_VALID(driver->api->send)) &&
    	(!IS_PTR_VALID(driver->api->recv)) &&
    	(!IS_PTR_VALID(driver->api->transact))) {
    		return RV_NOTIMPLEMENTED;
    }

    rv = RV_SUCCESS;
    if (IS_PTR_VALID(driver->api->initialize)) {
    	rv =  driver->api->initialize(driver);
    }

    if (RV_SUCCESS == rv) {
    	driver->state->is_initialized = true;
    }
    return rv;
}

retval_t channel_driver_deinitialize(const channel_driver_t * const driver)
{
	assert(driver);
    assert(driver->api);
    assert(driver->state);

    if (!driver->state->is_initialized) return RV_ILLEGAL;

    driver->state->is_initialized = false;

    if (IS_PTR_VALID(driver->api->deinitialize))
    	return driver->api->deinitialize(driver);
   	return RV_SUCCESS;
}
