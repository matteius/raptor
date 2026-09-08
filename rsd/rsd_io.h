/* Bounded per-client TCP backpressure; no sensor or media-clock policy. */
#ifndef RSD_IO_H
#define RSD_IO_H
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>

#define RSD_TCP_WRITE_TIMEOUT_MS 2000
#define RSD_CONTROL_WAIT_US 2000000

static inline int rsd_socket_write_timeout(int fd, unsigned int milliseconds)
{
	struct timeval timeout = {.tv_sec = milliseconds / 1000,
		.tv_usec = (milliseconds % 1000) * 1000};
	return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

/* 0: acquired; 1: retry later without consuming the request; -1: closed.
 * The shared event loop must never wait behind a client's network writer. */
static inline int rsd_control_trylock(pthread_mutex_t *lock, int fd,
				     int64_t now, int64_t *waiting_since)
{
	int ret = pthread_mutex_trylock(lock);
	if (!ret) {
		*waiting_since = 0;
		return 0;
	}
	if (ret == EBUSY) {
		if (!*waiting_since)
			*waiting_since = now ? now : 1;
		if (now - *waiting_since < RSD_CONTROL_WAIT_US)
			return 1;
	}
	/* A partial interleaved packet cannot be followed by a new packet. */
	shutdown(fd, SHUT_RDWR);
	*waiting_since = 0;
	return -1;
}
#endif
