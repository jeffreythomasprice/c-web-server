#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>

#include "tcp_socket_wrapper.h"

#include "../shared/log.h"

void *tcp_socket_wrapper_thread(void *data) {
	log_trace("tcp_socket_wrapper_thread start\n");
	tcp_socket_wrapper *sock_wrap = data;

	while (sock_wrap->running) {
		// a file descriptor set that contains just the one socket we're waiting on new connections for
		fd_set socket_fd_set;
		FD_ZERO(&socket_fd_set);
		FD_SET(sock_wrap->socket, &socket_fd_set);
		struct timeval socket_select_timeout;
		// 500 ms
		socket_select_timeout.tv_sec = 0;
		socket_select_timeout.tv_usec = 500000ull;
		int select_result = select(sock_wrap->socket + 1, &socket_fd_set, NULL, NULL, &socket_select_timeout);
		if (select_result < 0) {
			log_error("tcp_socket_wrapper_thread failed, error waiting on select\n");
			continue;
		}
		if (select_result == 0) {
			// just timed out, no need to log about this
			continue;
		}

		// no errors, we have an incoming connection

		// accept new incoming connection
		struct sockaddr_in request_address;
		socklen_t request_address_len = sizeof(struct sockaddr_in);
		int accepted_socket = accept(sock_wrap->socket, (struct sockaddr *)&request_address, &request_address_len);

		// some basic error checking
		if (accepted_socket == -1) {
			log_error("accepting incoming connection failed with %i\n", errno);
			continue;
		}
		if (request_address.sin_family == AF_INET6) {
			log_error("IPv6 unsupported\n");
			close(accepted_socket);
			continue;
		}
		if (request_address.sin_family != AF_INET) {
			log_error("unrecognized address family %i\n", request_address.sin_family);
			close(accepted_socket);
			continue;
		}

		char request_address_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &request_address.sin_addr, request_address_str, INET_ADDRSTRLEN);
		log_trace("incoming request from %s:%i\n", request_address_str, ntohs(request_address.sin_port));

		sock_wrap->callback(accepted_socket);
	}

	log_trace("tcp_socket_wrapper_thread done");
	return NULL;
}

int tcp_socket_wrapper_init(tcp_socket_wrapper *sock_wrap, char *address, uint16_t port, tcp_socket_wrapper_callback callback) {
	memset(sock_wrap, 0, sizeof(tcp_socket_wrapper));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	if (!address || !strcmp(address, "0.0.0.0")) {
		log_trace("tcp_socket_wrapper_init *:%i\n", port);
		addr.sin_addr.s_addr = INADDR_ANY;
	} else {
		log_trace("tcp_socket_wrapper_init %s:%i\n", address, port);
		log_error("TODO JEFF implement addr parsing\n");
		return 1;
	}
	addr.sin_port = htons(port);

	if (!callback) {
		log_error("tcp_socket_wrapper_init failed, callback is required\n");
		return 1;
	}
	sock_wrap->callback = callback;

	sock_wrap->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_wrap->socket == -1) {
		log_error("tcp_socket_wrapper_init failed, error creating socket, %s\n", strerror(errno));
		return 1;
	}
	if (bind(sock_wrap->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		log_error("tcp_socket_wrapper_init failed, failed to bind socket to port %i, %s\n", port, strerror(errno));
		close(sock_wrap->socket);
		sock_wrap->socket = 0;
		return 1;
	}
	// 2nd arg is number of connections that can be blocked waiting for the next accept
	if (listen(sock_wrap->socket, 10) < 0) {
		log_error("tcp_socket_wrapper_init failed, failed to listen on socket, %s\n", strerror(errno));
		close(sock_wrap->socket);
		sock_wrap->socket = 0;
		return 1;
	}

	sock_wrap->running = 1;
	if (pthread_create(&sock_wrap->thread, NULL, tcp_socket_wrapper_thread, sock_wrap)) {
		log_error("tcp_socket_wrapper_init failed, failed to make thread\n");
		close(sock_wrap->socket);
		sock_wrap->socket = 0;
		return 1;
	}
	sock_wrap->thread_is_init = 1;

	log_trace("tcp_socket_wrapper_init success\n");
	return 0;
}

int tcp_socket_wrapper_destroy(tcp_socket_wrapper *sock_wrap) {
	log_trace("tcp_socket_wrapper_destroy start");
	sock_wrap->running = 0;
	void *result;
	if (sock_wrap->thread_is_init && pthread_join(sock_wrap->thread, &result)) {
		log_error("tcp_socket_wrapper_destroy failed, pthread_join failed during shutdown\n");
	}
	if (sock_wrap->socket) {
		close(sock_wrap->socket);
		sock_wrap->socket = 0;
	}
	log_trace("tcp_socket_wrapper_destroy success");
	return 0;
}