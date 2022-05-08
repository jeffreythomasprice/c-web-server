#ifndef tcp_socket_wrapper_h
#define tcp_socket_wrapper_h

#include <netinet/in.h>

#include <pthread.h>

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * s is a socket handle that came from accept
 */
typedef void (*tcp_socket_wrapper_callback)(void *data, string *address, uint16_t port, int socket);

typedef struct {
	tcp_socket_wrapper_callback callback;
	void *callback_data;
	int socket;
	string address;
	uint16_t port;
	int running;
	int thread_is_init;
	pthread_t thread;
} tcp_socket_wrapper;

/**
 * @param addr a filled in socket address (e.g. the input to bind or the output of accept)
 * @param address optional, if present set to the address in addr->sin_addr, as a string appropriate for the family (i.e. IPv4 or IPv6
 * address)
 * @param port optiona, if present set to the port in addr->sin_port, in local byte order
 * @return 0 on success, non-0 on failure
 */
int get_sockaddr_info_str(struct sockaddr *addr, string *address, uint16_t *port);
/**
 * @param hostname set to the result of gethostname
 * @return 0 on success, non-0 on failure
 */
int get_hostname_str(string *hostname);
/**
 * @param hostname the hostname to get the address for
 * @param address set to the IP address of that hostname, as determined by gethostbyname
 * @return 0 on success, non-0 on failure
 */
int get_address_for_hostname_str(string *hostname, string *address);

/**
 * @param address the address to bind to which may be NULL or "0.0.0.0" to indicate binding to any address
 * @param port the port to bind to
 * @param callback the function to call on incoming TCP connections
 */
int tcp_socket_wrapper_init(tcp_socket_wrapper *sock_wrap, char *address, uint16_t port, tcp_socket_wrapper_callback callback,
							void *callback_data);
int tcp_socket_wrapper_dealloc(tcp_socket_wrapper *sock_wrap);
string *tcp_socket_wrapper_get_address(tcp_socket_wrapper *sock_wrap);
uint16_t tcp_socket_wrapper_get_port(tcp_socket_wrapper *sock_wrap);

#ifdef __cplusplus
}
#endif

#endif