#ifndef _REMOTE_CHANNEL_DRIVER_H_
#define _REMOTE_CHANNEL_DRIVER_H_

#include <canopus/drivers/channel.h>
#include <canopus/drivers/simusat/channel_posix.h>

/* A remote channel will forward send()/recv()/transact() to a remote server
 * It's sort of a channel over RPC.
 *
 * The transport, on which the requests are forwarded is another channel,
 * it could be of any type, but since this channels will be primarly used
 * for simulation we expect to have a tcp_channel as transport.
 */

extern const channel_driver_t remote_channel_driver;
extern const channel_driver_api_t remote_channel_driver_api;

extern const channel_driver_t remote_portmapped_channel_driver;
extern const channel_driver_api_t remote_portmapped_channel_driver_api;

typedef struct remote_channel_driver_config_t {
	channel_driver_config_t common;
} remote_channel_driver_config_t;

typedef struct remote_channel_driver_state_t {
	channel_driver_state_t common;
} remote_channel_driver_state_t;

typedef struct remote_channel_config_t {
	channel_config_t common;
    const char *const config_string;
    union {
    	struct {
    	    char *address;
    	    int   port;
    	};
    	const char *const endpoint_name;
    };
} remote_channel_config_t;

typedef struct remote_channel_state_t {
	channel_state_t common;
	channel_t * const transport;
} remote_channel_state_t;

typedef struct remote_portmapped_channel_driver_config_t {
	channel_driver_config_t common;
	channel_t *ch_portmapper;
} remote_portmapped_channel_driver_config_t;

#define DECLARE_CHANNEL_REMOTE(__config_string, __address, __port)					\
	(channel_t){																	\
		.config = (const channel_config_t *)&(remote_channel_config_t) {	\
			.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),	\
			.config_string = __config_string,										\
			.address = __address,													\
			.port    = __port,														\
		},																			\
		.state  = (channel_state_t *)&(remote_channel_state_t){						\
			.transport = &DECLARE_CHANNEL_TCP_CLIENT(NULL, 0),						\
		},																			\
		.driver = &remote_channel_driver,								\
	}

#define DECLARE_CHANNEL_REMOTE_PORTMAPPED(__config_string, __endpoint_name)			\
	(channel_t){																	\
		.config = (const channel_config_t *)&(remote_channel_config_t) {			\
			.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),		\
			.config_string = __config_string,										\
			.endpoint_name = __endpoint_name,										\
		},																			\
		.state  = (channel_state_t *)&(remote_channel_state_t){						\
			.transport = &DECLARE_CHANNEL_TCP_CLIENT(NULL, 0),						\
		},																			\
		.driver = &remote_portmapped_channel_driver,								\
	}

#endif /* _REMOTE_CHANNEL_DRIVER_H_ */
