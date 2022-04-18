#ifndef tcp_socket_wrapper_h
#define tcp_socket_wrapper_h

#include <stdint.h>

#include <pthread.h>

/**
 * s is a socket handle that came from accept
 */
typedef void (*tcp_socket_wrapper_callback)(int s);

typedef struct {
	tcp_socket_wrapper_callback callback;
	int socket;
	int running;
	int thread_is_init;
	pthread_t thread;
} tcp_socket_wrapper;

int tcp_socket_wrapper_init(tcp_socket_wrapper *sock_wrap, char *address, uint16_t port, tcp_socket_wrapper_callback callback);
int tcp_socket_wrapper_destroy(tcp_socket_wrapper *sock_wrap);

#endif