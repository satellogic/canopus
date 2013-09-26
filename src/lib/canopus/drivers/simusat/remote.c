#include <canopus/drivers/simusat/remote.h>
#include <canopus/drivers/simusat/channel_posix.h>
#include <canopus/logging.h>
#include <canopus/assert.h>
#include <string.h>

enum {
	PORTMAPPER_COMMAND_PING = 0,
	PORTMAPPER_COMMAND_LIST,
	PORTMAPPER_COMMAND_REGISTER,
	PORTMAPPER_COMMAND_GET_STATUS,
};

enum {
	REMOTE_COMMAND_GET_NAME = 0,
	REMOTE_COMMAND_OPEN,
	REMOTE_COMMAND_CLOSE,
	REMOTE_COMMAND_SEND,
	REMOTE_COMMAND_RECV,
	REMOTE_COMMAND_TRANSACT,
};

static retval_t remote_portmapped_initialize(const channel_driver_t * const driver) {
	retval_t rv;
	frame_t cmd_ping = DECLARE_FRAME_BYTES(PORTMAPPER_COMMAND_PING);
	frame_t answer = DECLARE_FRAME_SPACE(6);
	remote_portmapped_channel_driver_config_t *d_config = (remote_portmapped_channel_driver_config_t *)driver->config;
	channel_t *ch_portmapper = d_config->ch_portmapper;
	uint8_t len;
	uint8_t *pong;

	rv = channel_open(ch_portmapper);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_send(ch_portmapper, &cmd_ping);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(ch_portmapper, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset_for_reading(&answer);

	rv = frame_get_u8(&answer, &len);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_data_pointer(&answer, (void**)&pong, len);
	if (RV_SUCCESS != rv) return rv;

	rv = RV_SUCCESS;
	if (0 != memcmp(pong, "PONG1", 5)) rv = RV_ERROR;
	return rv;
}

static retval_t remote_deinitialize(const channel_driver_t * const driver) {
	return RV_SUCCESS;
}

static retval_t remote_portmapped_deinitialize(const channel_driver_t * const driver) {
	remote_portmapped_channel_driver_config_t *d_config = (remote_portmapped_channel_driver_config_t *)driver->config;
	channel_t *simusat = d_config->ch_portmapper;

	return channel_close(simusat);
}

static retval_t _remote_open(const channel_t * const channel, char *address, uint16_t tcp_port, const char * const config_string) {
	remote_channel_state_t *c_state;
	frame_t cmd_size = DECLARE_FRAME_SPACE(301);
	frame_t answer = DECLARE_FRAME_SPACE(1);
	tcp_channel_config_t *transport_config;
	size_t config_len;
	retval_t rv;
	uint8_t _remote_rv;

	c_state  = (remote_channel_state_t*)channel->state;

	transport_config = (tcp_channel_config_t*)c_state->transport->config;
	transport_config->address = address;
	transport_config->port    = tcp_port;

	rv =  channel_open(c_state->transport);
	if (RV_SUCCESS != rv) return rv;

	config_len = strlen(config_string);
	frame_put_u8(&cmd_size, REMOTE_COMMAND_OPEN);
	frame_put_u8(&cmd_size, config_len);
	frame_put_data(&cmd_size, config_string, config_len);
	frame_reset_for_reading(&cmd_size);

	//	channel_lock(c_state->transport);
	rv = channel_send(c_state->transport, &cmd_size);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset(&answer);
	rv = frame_get_u8(&answer, &_remote_rv);
	if (RV_SUCCESS != rv) return rv;
	return (retval_t)_remote_rv;
}

static retval_t remote_open(const channel_t * const channel) {
	remote_channel_config_t *c_config;

	c_config = (remote_channel_config_t*)channel->config;

	return _remote_open(channel, c_config->address, c_config->port, c_config->config_string);
}

retval_t portmapper_get_status(const channel_t * const ch_portmapper, const char *device_name, uint16_t *tcp_port, bool *running) {
	frame_t cmd = DECLARE_FRAME_SPACE(301);
	frame_t answer = DECLARE_FRAME_SPACE(3);
	size_t device_name_len = strlen(device_name);
	uint8_t _running;
	retval_t rv;

	(void)frame_put_u8(&cmd, PORTMAPPER_COMMAND_GET_STATUS);
	(void)frame_put_u8(&cmd, device_name_len);
	rv = frame_put_data(&cmd, device_name, device_name_len);
	if (RV_SUCCESS != rv) return rv;
	frame_reset_for_reading(&cmd);

	rv = channel_send(ch_portmapper, &cmd);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(ch_portmapper, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset(&answer);

	rv = frame_get_u16(&answer, tcp_port);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u8(&answer, &_running);
	if (RV_SUCCESS == rv) *running = _running == 1;
	return rv;
}

static retval_t remote_portmapped_open(const channel_t * const channel) {
	remote_portmapped_channel_driver_config_t *d_config;
	remote_channel_config_t *c_config;
	remote_channel_config_t *portmapper_config;
	uint16_t tcp_port;
	bool running;
	retval_t rv;

	c_config = (remote_channel_config_t*)channel->config;

	d_config = (remote_portmapped_channel_driver_config_t*)channel->driver->config;
	portmapper_get_status(d_config->ch_portmapper, c_config->endpoint_name, &tcp_port, &running);
	if (running) {
		portmapper_config = (remote_channel_config_t*)d_config->ch_portmapper->config;
		rv = _remote_open(channel, portmapper_config->address, tcp_port, c_config->config_string);
	} else {
		rv = RV_ERROR;
	}
	return rv;
}

static retval_t remote_close(const channel_t * const channel) {
	frame_t cmd = DECLARE_FRAME_SPACE(1);
	frame_t answer = DECLARE_FRAME_SPACE(1);
	remote_channel_state_t *c_state;
	retval_t rv;
	uint8_t _remote_rv;

	c_state  = (remote_channel_state_t*)channel->state;

	frame_put_u8(&cmd, REMOTE_COMMAND_CLOSE);
	frame_reset_for_reading(&cmd);

//	channel_lock(c_state->transport);
	rv = channel_send(c_state->transport, &cmd);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset(&answer);
	rv = frame_get_u8(&answer, &_remote_rv);
	if (RV_SUCCESS != rv) return rv;
	if (RV_SUCCESS != (retval_t)_remote_rv) return (retval_t)_remote_rv;

	return channel_close(c_state->transport);
}

static retval_t remote_send(const channel_t * const channel, frame_t * const send_frame, const size_t count) {
	retval_t rv;
	uint8_t _remote_rv = 0;
	frame_t cmd_size = DECLARE_FRAME_SPACE(5);
	frame_t answer = DECLARE_FRAME_SPACE(5);
	remote_channel_state_t *c_state;
	uint32_t written;

	c_state  = (remote_channel_state_t*)channel->state;

	frame_put_u8(&cmd_size, REMOTE_COMMAND_SEND);
	frame_put_u32(&cmd_size, _frame_available_data(send_frame));
	frame_reset_for_reading(&cmd_size);

	//	channel_lock(c_state->transport);
	rv = channel_send(c_state->transport, &cmd_size);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_send(c_state->transport, send_frame);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset(&answer);
	rv = frame_get_u8(&answer, &_remote_rv);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u32(&answer, &written);
	if (RV_SUCCESS != rv) return rv;

	return (retval_t)_remote_rv;
}

static retval_t remote_recv(const channel_t * const channel, frame_t * const recv_frame, const size_t count) {
	retval_t rv;
	uint8_t _remote_rv;
	frame_t cmd_size = DECLARE_FRAME_SPACE(5);
	frame_t answer = DECLARE_FRAME_SPACE(5);
	remote_channel_state_t *c_state;
	uint32_t answer_len;

	c_state  = (remote_channel_state_t*)channel->state;

	frame_put_u8(&cmd_size, REMOTE_COMMAND_RECV);
	frame_put_u32(&cmd_size, _frame_available_space(recv_frame));
	frame_reset_for_reading(&cmd_size);

//	channel_lock(c_state->transport);
	rv = channel_send(c_state->transport, &cmd_size);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset(&answer);
	rv = frame_get_u8(&answer, &_remote_rv);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u32(&answer, &answer_len);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, recv_frame);
	return (retval_t)_remote_rv;
}

static retval_t remote_transact(
		const channel_t * const channel,
        frame_t * const send_frame, uint32_t delay_ms, frame_t * const recv_frame,
        const size_t send_bytes, const size_t recv_bytes)
{

	retval_t rv;
	uint8_t _remote_rv;
	frame_t cmd_size = DECLARE_FRAME_SPACE(13);
	frame_t answer = DECLARE_FRAME_SPACE(9);
	remote_channel_state_t *c_state;
	uint32_t written, answer_len;

	c_state  = (remote_channel_state_t*)channel->state;

	frame_put_u8(&cmd_size, REMOTE_COMMAND_TRANSACT);
	frame_put_u32(&cmd_size, _frame_available_data(send_frame));
	frame_put_u32(&cmd_size, delay_ms);
	frame_put_u32(&cmd_size, _frame_available_space(recv_frame));

	frame_reset_for_reading(&cmd_size);

	//	channel_lock(c_state->transport);
	rv = channel_send(c_state->transport, &cmd_size);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_send(c_state->transport, send_frame);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, &answer);
	if (RV_SUCCESS != rv) return rv;

	frame_reset(&answer);
	rv = frame_get_u8(&answer, &_remote_rv);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u32(&answer, &written);
	if (RV_SUCCESS != rv) return rv;

	rv = frame_get_u32(&answer, &answer_len);
	if (RV_SUCCESS != rv) return rv;

	rv = channel_recv(c_state->transport, recv_frame);
	return (retval_t)_remote_rv;
}

const channel_driver_api_t remote_channel_driver_api = {
	.initialize = INVALID_PTR,
	.deinitialize = remote_deinitialize,
	.open = remote_open,
	.close = remote_close,
	.send = remote_send,
	.recv = remote_recv,
	.transact = remote_transact,
};

const channel_driver_api_t remote_portmapped_channel_driver_api = {
	.initialize = remote_portmapped_initialize,
	.deinitialize = remote_portmapped_deinitialize,
	.open = remote_portmapped_open,
	.close = remote_close,
	.send = remote_send,
	.recv = remote_recv,
	.transact = remote_transact,
};

remote_channel_driver_config_t remote_channel_driver_config = {};

remote_portmapped_channel_driver_config_t remote_portmapped_channel_driver_config = {
	.ch_portmapper = &DECLARE_CHANNEL_REMOTE("MAPPER", "localhost", 11111),
};

const channel_driver_t remote_channel_driver = DECLARE_CHANNEL_DRIVER(&remote_channel_driver_api, &remote_channel_driver_config, remote_channel_driver_state_t);
const channel_driver_t remote_portmapped_channel_driver = DECLARE_CHANNEL_DRIVER(&remote_portmapped_channel_driver_api, &remote_portmapped_channel_driver_config, remote_channel_driver_state_t);
