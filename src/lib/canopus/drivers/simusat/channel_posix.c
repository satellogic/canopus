#include <canopus/drivers/simusat/channel_posix.h>
#include <canopus/logging.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifdef __USE_POSIX
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#endif /* __USE_POSIX */
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#define EINTR_DELAY 1
#define CONNECT_TIMEOUT_SEC 3
#define USE_MASTER_FD_FROM_STATE -1
#define NO_INCOMING_CONNETION_YET -1

static int
socket_errno_timeouted()
{
    /* since we (expect to) use non-blocking socket, ECONNREFUSED is temporary,
     * meaning we can re-try the operation later. */
#ifdef __MINGW32__
    return WSAETIMEDOUT == WSAGetLastError() || WSAECONNREFUSED == WSAGetLastError();
#else /* BSD */
    return ETIMEDOUT == errno || ECONNREFUSED == errno;
#endif
}

#define SET_CHANNEL_STATE_FD(__channel, __fd)		((fd_channel_state_t*)(__channel->state))->fd = (__fd)

static retval_t fd_open(
        const channel_t * const channel)
{
    const fd_channel_config_t * const l_config =
        (const fd_channel_config_t * const) channel->config;

    SET_CHANNEL_STATE_FD(channel, l_config->fd);
    return RV_SUCCESS;
}

static retval_t file_open(
        const channel_t * const link)
{
    int fd;
    const file_channel_config_t * const l_config =
        (const file_channel_config_t * const) link->config;

    fd = open(l_config->filename, l_config->flags, l_config->mode);
    if (-1 == fd) return RV_ERROR;

    SET_CHANNEL_STATE_FD(link, fd);
    return RV_SUCCESS;
}

static retval_t tcp_client_open(
        const channel_t * const link)
{
    int fd, ret;
    const tcp_channel_config_t * const l_config =
        (const tcp_channel_config_t * const) link->config;
    struct sockaddr_in addr;
    struct hostent *he;

    he = gethostbyname(l_config->address);
    if (NULL == he) return RV_ERROR;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) return RV_ERROR;

    memset(&addr, 0, sizeof addr);
    addr.sin_family = he->h_addrtype;
    addr.sin_port   = htons(l_config->port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    /* TODO non blocking ... */
    while (1) {
    	ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (ret == -1 && EINTR == errno) {
            vTaskDelay(EINTR_DELAY);
        } else {
        	break;
        }
    };

    if (-1 == ret) {
        log_report_fmt(LOG_SOCKET, "connect(%s:%d/tcp) failed\n", l_config->address, l_config->port);
    	close(fd);
        if (socket_errno_timeouted()) return RV_TIMEOUT;
    	return RV_ERROR;
    }

    SET_CHANNEL_STATE_FD(link, fd);
    return RV_SUCCESS;
}

static retval_t wait_connection(const channel_t * const link, int master_fd, uint32_t timeout_s) {
	fd_channel_state_t *l_state = (fd_channel_state_t*)link->state;
    struct timeval timeout;
    fd_set rdfds;
    int rv;

    if (master_fd == USE_MASTER_FD_FROM_STATE) {
    	master_fd = l_state->master_fd;
    } else {
    	l_state->master_fd = master_fd;

		rv = listen(master_fd, 1);
		if (-1 == rv) {
			close(master_fd);
			return RV_ERROR;
		}
    }

    timeout.tv_sec = timeout_s;
    timeout.tv_usec = 0;
    FD_ZERO(&rdfds);
    FD_SET(master_fd, &rdfds);

    while (1) {
    	rv = select(master_fd + 1, &rdfds, NULL, NULL, &timeout);
        if (rv == -1 && EINTR == errno) {
            vTaskDelay(EINTR_DELAY);
        } else {
        	break;
        }
    };

    if (!FD_ISSET(master_fd, &rdfds)) {
        SET_CHANNEL_STATE_FD(link, NO_INCOMING_CONNETION_YET);
        return RV_TIMEOUT;
    }

    while (1) {
    	rv = accept(master_fd, NULL, NULL);
        if (rv == -1 && EINTR == errno) {
            vTaskDelay(EINTR_DELAY);
        } else {
        	break;
        }
    };

    if (-1 == rv) return RV_ERROR;
	close(master_fd);

    SET_CHANNEL_STATE_FD(link, rv);
    return RV_SUCCESS;
}

static retval_t tcp_server_open(
        const channel_t * const link)
{
    int fd, ret;
    const tcp_channel_config_t * const l_config = (const tcp_channel_config_t * const) link->config;
    struct sockaddr_in addr;
    struct hostent *he;

    he = gethostbyname(l_config->address);
    if (NULL == he) return RV_ERROR;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) return RV_ERROR;

    ret = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&ret, sizeof(ret));
    (void)(ret == 0);	/* we just keep going if this fails, what can we do? */

    memset(&addr, 0, sizeof addr);
    addr.sin_family = he->h_addrtype;
    addr.sin_port   = htons(l_config->port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (-1 == ret) {
    	close(fd);
    	return RV_ERROR;
    }

    return wait_connection(link, fd, CONNECT_TIMEOUT_SEC);
}

#ifdef __USE_POSIX
static retval_t unix_socket_client_open(
        const channel_t * const link)
{
    int fd, ret;
    const unix_socket_channel_config_t * const l_config =
        (const unix_socket_channel_config_t * const) link->config;
    struct sockaddr_un addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) return RV_ERROR;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd) return RV_ERROR;

    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    memcpy(&addr.sun_path, l_config->name, sizeof(addr.sun_path));

    while (1) {
    	ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (ret == -1 && EINTR == errno) {
            vTaskDelay(EINTR_DELAY);
            /* FIXME timeout? */
        } else {
        	break;
        }
    };

    if (-1 == ret) {
        log_report_fmt(LOG_SOCKET, "connect(%s/unix) failed\n", l_config->name);
    	close(fd);
        if (socket_errno_timeouted()) return RV_TIMEOUT;
    	return RV_ERROR;
    }

    SET_CHANNEL_STATE_FD(link, fd);
    return RV_SUCCESS;
}

static retval_t unix_socket_server_open(
        const channel_t * const link)
{
    int fd, ret;
    const unix_socket_channel_config_t * const l_config =
        (const unix_socket_channel_config_t * const) link->config;
    struct sockaddr_un addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd) return RV_ERROR;

    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    memcpy(&addr.sun_path, l_config->name, sizeof(addr.sun_path));

    (void)unlink(l_config->name);
    log_report_fmt(LOG_SOCKET, "Connect to unix socket %s\n", l_config->name);

    ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (-1 == ret) {
    	close(fd);
    	return RV_ERROR;
    }

    return wait_connection(link, fd, CONNECT_TIMEOUT_SEC);
}
#endif /* __USE_POSIX */

static retval_t fd_close(
        const channel_t * const link)
{
    fd_channel_state_t * const c_state =
        (fd_channel_state_t * const) link->state;

    close(c_state->fd);
    return RV_SUCCESS;
}

typedef ssize_t read_t(int __fd, void *__buf, size_t __nbytes, int _flags);
typedef ssize_t write_t(int __fd, __const void *__buf, size_t __n, int _flags);

static retval_t _write_or_send(
        const channel_t * const link,
        frame_t * const send_frame,
        const size_t count,
        write_t *_write)
{
	fd_channel_state_t * const l_state = (fd_channel_state_t * const) link->state;

	void *buf;
	retval_t rv;
	ssize_t actual_count;

    if (l_state->fd == NO_INCOMING_CONNETION_YET) {
    	 rv = wait_connection(link, USE_MASTER_FD_FROM_STATE, 0);
    	 if (rv != RV_SUCCESS) return rv;
    }

    if (0 == count) return RV_SUCCESS;

    rv = frame_get_data_pointer(send_frame, &buf, count);
    if (RV_SUCCESS != rv) return rv;

    /* when running under FreeRTOS POSIX simulator, send(3) might be interrupted by a signal,
       returning -1 and errno == EINTR. We want this recv() function to be blocking, thus this loop here */
    while (1) {
        actual_count = _write(l_state->fd, buf, count, 0); // TODO catch SIGPIPE -> reconnect
        if (actual_count == -1 && EINTR == errno) {
            vTaskDelay(EINTR_DELAY);
        } else {
        	break;
        }
    };

    if (actual_count < 0 || actual_count > count) {
    	if (EPIPE == errno) l_state->common.is_open = false;
    	return RV_ERROR;
    }

    frame_advance(send_frame, actual_count);
    if (actual_count < count) return RV_PARTIAL;
    return RV_SUCCESS;
}

static retval_t fd_send(
        const channel_t * const link,
        frame_t * const send_frame,
        const size_t count) {
	return _write_or_send(link, send_frame, count, (void*)write);
}

static retval_t sock_send(
        const channel_t * const link,
        frame_t * const send_frame,
        const size_t count) {
	return _write_or_send(link, send_frame, count, send);
}

static retval_t _read_or_recv(
        const channel_t * const link,
        frame_t * const recv_frame,
        const size_t count,
        read_t *_read)
{
    fd_channel_state_t *l_state = (fd_channel_state_t *)link->state;

	retval_t rv;
	void *buf;
	ssize_t actual_count;

    if (l_state->fd == NO_INCOMING_CONNETION_YET) {
    	 rv = wait_connection(link, USE_MASTER_FD_FROM_STATE, 0);
    	 if (rv != RV_SUCCESS) return rv;
    }

    if (0 == count) return RV_SUCCESS;

    rv = frame_get_data_pointer(recv_frame, &buf, count);
    if (rv != RV_SUCCESS) return rv;

    /* when running under FreeRTOS POSIX simulator, read(3) might be interrupted by a signal,
       returning -1 and errno == EINTR. We want this recv() function to be blocking, thus this loop here */
    while (1) {
    	actual_count = _read(l_state->fd, buf, count, 0);
        if (actual_count == -1 && EINTR == errno) {
            vTaskDelay(EINTR_DELAY);
        } else {
        	break;
        }
    };

    if (-1 == actual_count) {
    	if (EPIPE == errno) l_state->common.is_open = false;
    	return RV_ERROR;
    }

    frame_advance(recv_frame, actual_count);
    if (actual_count < count) return RV_PARTIAL;
    return RV_SUCCESS;
}

static retval_t fd_recv(
        const channel_t * const link,
        frame_t * const recv_frame,
        const size_t count) {
	return _read_or_recv(link, recv_frame, count, (void*)read);
}

static retval_t sock_recv(
        const channel_t * const link,
        frame_t * const recv_frame,
        const size_t count) {
	return _read_or_recv(link, recv_frame, count, recv);
}

const channel_driver_api_t fd_channel_driver_api = {
	.initialize   = INVALID_PTR,
	.deinitialize = INVALID_PTR,
	.open     = &fd_open,
	.close    = &fd_close,
	.send     = &fd_send,
	.recv     = &fd_recv,
    .transact = INVALID_PTR
};

const channel_driver_api_t file_channel_driver_api = {
	.initialize   = INVALID_PTR,
	.deinitialize = INVALID_PTR,
	.open     = &file_open,
	.close    = &fd_close,
	.send     = &fd_send,
	.recv     = &fd_recv,
    .transact = INVALID_PTR
};

const channel_driver_api_t tcp_client_channel_driver_api = {
	.initialize   = INVALID_PTR,
	.deinitialize = INVALID_PTR,
	.open     = &tcp_client_open,
	.close    = &fd_close,
	.send     = &sock_send,
	.recv     = &sock_recv,
    .transact = INVALID_PTR
};

const channel_driver_api_t tcp_server_channel_driver_api = {
	.initialize   = INVALID_PTR,
	.deinitialize = INVALID_PTR,
	.open     = &tcp_server_open,
	.close    = &fd_close,
	.send     = &sock_send,
	.recv     = &sock_recv,
    .transact = INVALID_PTR
};

#ifdef __USE_POSIX
const channel_driver_api_t unix_socket_client_channel_driver_api = {
	.initialize   = INVALID_PTR,
	.deinitialize = INVALID_PTR,
	.open     = &unix_socket_client_open,
	.close    = &fd_close,
	.send     = &sock_send,
	.recv     = &sock_recv,
    .transact = INVALID_PTR
};

const channel_driver_api_t unix_socket_server_channel_driver_api = {
	.initialize   = INVALID_PTR,
	.deinitialize = INVALID_PTR,
	.open     = &unix_socket_server_open,
	.close    = &fd_close,
	.send     = &sock_send,
	.recv     = &sock_recv,
    .transact = INVALID_PTR
};
#endif /* __USE_POSIX */

/* Driver instances. This will probably move to some initialization file in the application */

const channel_driver_t fd_channel_driver = DECLARE_CHANNEL_DRIVER(&fd_channel_driver_api, NULL, channel_driver_state_t);
const channel_driver_t file_channel_driver = DECLARE_CHANNEL_DRIVER(&file_channel_driver_api, NULL, channel_driver_state_t);
const channel_driver_t tcp_client_channel_driver = DECLARE_CHANNEL_DRIVER(&tcp_client_channel_driver_api, NULL, channel_driver_state_t);
const channel_driver_t tcp_server_channel_driver = DECLARE_CHANNEL_DRIVER(&tcp_server_channel_driver_api, NULL, channel_driver_state_t);

#ifdef __USE_POSIX /* (phil) mingw32 kludge: no unix-sockets. */
const channel_driver_t unix_socket_client_channel_driver = DECLARE_CHANNEL_DRIVER(&unix_socket_client_channel_driver_api, NULL, channel_driver_state_t);
const channel_driver_t unix_socket_server_channel_driver = DECLARE_CHANNEL_DRIVER(&unix_socket_server_channel_driver_api, NULL, channel_driver_state_t);
#endif
