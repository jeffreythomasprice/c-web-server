#ifndef tcp_socket_wrapper_h
#define tcp_socket_wrapper_h

#include <stdint.h>

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * s is a socket handle that came from accept
 */
typedef void (*tcp_socket_wrapper_callback)(void *data, char *address, uint16_t port, int socket);

typedef struct {
	tcp_socket_wrapper_callback callback;
	void *callback_data;
	int socket;
	int running;
	int thread_is_init;
	pthread_t thread;
} tcp_socket_wrapper;

/**
 * @param address the address to bind to which may be NULL or "0.0.0.0" to indicate binding to any address
 * @param port the port to bind to
 * @param callback the function to call on incoming TCP connections
 */
int tcp_socket_wrapper_init(tcp_socket_wrapper *sock_wrap, char *address, uint16_t port, tcp_socket_wrapper_callback callback,
							void *callback_data);
int tcp_socket_wrapper_dealloc(tcp_socket_wrapper *sock_wrap);

#ifdef __cplusplus
}
#endif

#endif