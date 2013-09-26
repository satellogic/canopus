#ifndef _FILE_CHANNEL_DRIVER_H_
#define _FILE_CHANNEL_DRIVER_H_

#include <canopus/drivers/channel.h>

#define CHANNEL_POSIX_DEFAULT_TIMEOUT_ms	100
extern const channel_driver_t fd_channel_driver;
extern const channel_driver_t file_channel_driver;
extern const channel_driver_t tcp_client_channel_driver;
extern const channel_driver_t tcp_server_channel_driver;

#ifdef __USE_POSIX /* (phil) mingw32 kludge: no unix-sockets. */
extern const channel_driver_t unix_socket_client_channel_driver;
extern const channel_driver_t unix_socket_server_channel_driver;
#endif

typedef struct fd_channel_config_t {
	channel_config_t common;
    /* The file descriptor on which to operate */
    int    fd;
} fd_channel_config_t;

typedef struct file_channel_config_t {
	channel_config_t common;
    char *filename;
    int   flags;	/* O_RDWR, O_WRONLY, O_RDONLY, O_CREAT, etc... */
    int   mode;		/* unix file mode, as required by O_CREAT */
} file_channel_config_t;

typedef struct tcp_channel_config_t {
	channel_config_t common;
    char *address;
    int   port;
} tcp_channel_config_t;

typedef struct fd_channel_state_t {
	channel_state_t common;
    int  fd;
    int  master_fd;
} fd_channel_state_t;

/**
 * Channel Link Driver
 **/

extern const channel_driver_api_t fd_channel_driver_api;
extern const channel_driver_api_t file_channel_driver_api;
extern const channel_driver_api_t tcp_client_channel_driver_api;
extern const channel_driver_api_t tcp_server_channel_driver_api;

#define DECLARE_CHANNEL_FD(_fd)														\
	(channel_t){																	\
			.config = (const channel_config_t *)&(fd_channel_config_t) {			\
				.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),	\
				.fd = _fd															\
			},																		\
		.state  = (channel_state_t *)&(fd_channel_state_t){},						\
		.driver = &fd_channel_driver,												\
	}

/* DECLARE_CHANNEL_FILE(_filename, _flags, _mode)
 *  int   flags;	 O_RDWR, O_WRONLY, O_RDONLY, O_CREAT, etc...
 *  int   mode;		 unix file mode, as required by O_CREAT
 */
#define DECLARE_CHANNEL_FILE(_filename, _flags, _mode)							\
	(channel_t){																\
		.config = (const channel_config_t *)&(file_channel_config_t){			\
			.common   = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),	\
			.filename = _filename,												\
			.flags    = _flags,													\
			.mode     = _mode,													\
		},																		\
		.state  = (channel_state_t *)&(fd_channel_state_t){},					\
		.driver = &file_channel_driver,											\
	}

#define DECLARE_CHANNEL_TCP_CLIENT(_hostaddr, _port)							\
	(channel_t){																\
		.config = (const channel_config_t *)&(tcp_channel_config_t){			\
			.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),	\
			.address = _hostaddr,												\
			.port    = _port													\
		},																		\
		.state  = (channel_state_t *)&(fd_channel_state_t){},				\
		.driver = &tcp_client_channel_driver,								\
	}

#define DECLARE_CHANNEL_TCP_SERVER(_localaddr, _port)							\
	(channel_t){																\
			.config = (const channel_config_t *)&(tcp_channel_config_t) {		\
				.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms)	\
				.address = _localaddr,											\
				.port    = _port												\
			},																	\
		.state  = (channel_state_t *)&(fd_channel_state_t){},					\
		.driver = &tcp_server_channel_driver,									\
	}

#ifdef __USE_POSIX
typedef struct unix_socket_channel_config_t {
	channel_config_t common;
    char *name;
} unix_socket_channel_config_t;

extern const channel_driver_api_t unix_socket_client_channel_driver_api;
extern const channel_driver_api_t unix_socket_server_channel_driver_api;

#define DECLARE_CHANNEL_UNIX_SOCKET_CLIENT(_name)									\
	(channel_t){																	\
			.config = (const channel_config_t *)&(unix_socket_channel_config_t) {	\
				.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),	\
				.name = _name														\
			},																		\
		.state  = (channel_state_t *)&(fd_channel_state_t){},						\
		.driver = &unix_socket_client_channel_driver,								\
	}

#define DECLARE_CHANNEL_UNIX_SOCKET_SERVER(_name)									\
	(channel_t){																	\
			.config = (const channel_config_t *)&(unix_socket_channel_config_t) {	\
				.common  = DECLARE_CHANNEL_CONFIG(0, CHANNEL_FLAG_NO_AUTO_LOCK, CHANNEL_POSIX_DEFAULT_TIMEOUT_ms),	\
				.name = _name														\
			},																		\
		.state  = (channel_state_t *)&(fd_channel_state_t){},						\
		.driver = &unix_socket_server_channel_driver,								\
	}

#endif /* __USE_POSIX */

#endif	/* _FILE_CHANNEL_DRIVER_H_ */
