#include "greatest.h"
#include "../rsd/rsd_io.h"
#include <string.h>
#include <unistd.h>

TEST rsd_control_defers_without_blocking_or_consuming_deadline(void)
{
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	int64_t pending = 0;
	ASSERT_EQ(0, pthread_mutex_lock(&lock));
	ASSERT_EQ(1, rsd_control_trylock(&lock, -1, 100, &pending));
	ASSERT_EQ(100, pending);
	ASSERT_EQ(1, rsd_control_trylock(&lock, -1, 1000000, &pending));
	ASSERT_EQ(100, pending);
	ASSERT_EQ(0, pthread_mutex_unlock(&lock));
	ASSERT_EQ(0, rsd_control_trylock(&lock, -1, 1000001, &pending));
	ASSERT_EQ(0, pending);
	ASSERT_EQ(0, pthread_mutex_unlock(&lock));
	ASSERT_EQ(0, pthread_mutex_destroy(&lock));
	PASS();
}

TEST rsd_control_deadline_closes_only_stalled_connection(void)
{
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	int fd[2]; char byte;
	int64_t pending = 0;
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
	ASSERT_EQ(0, pthread_mutex_lock(&lock));
	ASSERT_EQ(1, rsd_control_trylock(&lock, fd[0], 100, &pending));
	ASSERT_EQ(-1, rsd_control_trylock(&lock, fd[0], 100 + RSD_CONTROL_WAIT_US, &pending));
	ASSERT_EQ(0, pending);
	ASSERT_EQ(0, recv(fd[1], &byte, 1, MSG_DONTWAIT));
	ASSERT_EQ(0, pthread_mutex_unlock(&lock));
	ASSERT_EQ(0, pthread_mutex_destroy(&lock));
	close(fd[0]); close(fd[1]);
	PASS();
}

TEST rsd_socket_stall_has_finite_write_timeout(void)
{
	int fd[2],size=4096;
	char data[4096]; struct timeval timeout; socklen_t len=sizeof(timeout);
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
	ASSERT_EQ(0, setsockopt(fd[0], SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)));
	ASSERT_EQ(0, rsd_socket_write_timeout(fd[0], 50));
	ASSERT_EQ(0, getsockopt(fd[0], SOL_SOCKET, SO_SNDTIMEO, &timeout, &len));
	ASSERT(timeout.tv_sec || timeout.tv_usec);
	memset(data, 0xa5, sizeof(data));
	while(send(fd[0], data, sizeof(data), MSG_DONTWAIT|MSG_NOSIGNAL)>0) {}
	ASSERT(errno==EAGAIN || errno==EWOULDBLOCK);
	ASSERT_EQ(-1, send(fd[0], data, sizeof(data), MSG_NOSIGNAL));
	ASSERT(errno==EAGAIN || errno==EWOULDBLOCK);
	close(fd[0]); close(fd[1]);
	PASS();
}

SUITE(rsd_io_suite)
{
	RUN_TEST(rsd_control_defers_without_blocking_or_consuming_deadline);
	RUN_TEST(rsd_control_deadline_closes_only_stalled_connection);
	RUN_TEST(rsd_socket_stall_has_finite_write_timeout);
}
