#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>

#include "tcp_socket_wrapper.h"

#include "../shared/log.h"

// private
void get_inaddr_4_str(struct in_addr *addr, string *result) {
	char temp[INET_ADDRSTRLEN + 1];
	inet_ntop(AF_INET, addr, temp, sizeof(temp));
	string_set_cstr(result, temp);
}

// private
void get_inaddr_6_str(struct in6_addr *addr, string *result) {
	char temp[INET6_ADDRSTRLEN + 1];
	inet_ntop(AF_INET6, addr, temp, sizeof(temp));
	string_set_cstr(result, temp);
}

int get_sockaddr_info_str(struct sockaddr *addr, string *address, uint16_t *port) {
	int result = 0;
	switch (((struct sockaddr_in *)addr)->sin_family) {
	case AF_INET:
		if (address) {
			get_inaddr_4_str(&((struct sockaddr_in *)addr)->sin_addr, address);
		}
		if (port) {
			*port = ntohs(((struct sockaddr_in *)addr)->sin_port);
		}
		break;
	case AF_INET6:
		if (address) {
			get_inaddr_6_str(&((struct sockaddr_in6 *)addr)->sin6_addr, address);
		}
		if (port) {
			*port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
		}
		break;
	default:
		result = 1;
		break;
	}
	return result;
}

int get_hostname_str(string *hostname) {
	char tmp[HOST_NAME_MAX];
	if (gethostname(tmp, HOST_NAME_MAX)) {
		return 1;
	}
	string_set_cstr(hostname, tmp);
	return 0;
}

int get_address_for_hostname_str(string *hostname, string *address) {
	struct hostent *he = gethostbyname(string_get_cstr(hostname));
	int result = 0;
	switch (he->h_addrtype) {
	case AF_INET:
		get_inaddr_4_str((struct in_addr *)he->h_addr_list[0], address);
		break;
	case AF_INET6:
		get_inaddr_6_str((struct in6_addr *)he->h_addr_list[0], address);
		break;
	default:
		result = 1;
		break;
	}
	return result;
}

// private
void *tcp_socket_wrapper_thread(void *data) {
	log_trace("tcp_socket_wrapper_thread start\n");
	tcp_socket_wrapper *sock_wrap = data;

	string address;
	string_init(&address);
	uint16_t port;

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
		union {
			struct sockaddr addr;
			struct sockaddr_in addr4;
			struct sockaddr_in6 addr6;
		} request_address;
		socklen_t request_address_len = sizeof(request_address);
		int accepted_socket = accept(sock_wrap->socket, &request_address.addr, &request_address_len);

		// some basic error checking
		if (accepted_socket == -1) {
			log_error("accepting incoming connection failed with %i\n", errno);
			continue;
		}

		if (get_sockaddr_info_str((struct sockaddr *)&request_address, &address, &port)) {
			log_error("error parsing address for incoming connection\n");
			close(accepted_socket);
			continue;
		}
		log_trace("incoming request from %s:%i\n", string_get_cstr(&address), port);

		sock_wrap->callback(sock_wrap->callback_data, &address, port, accepted_socket);
	}

	string_dealloc(&address);

	log_trace("tcp_socket_wrapper_thread done\n");
	return NULL;
}

int tcp_socket_wrapper_init(tcp_socket_wrapper *sock_wrap, char *address, uint16_t port, tcp_socket_wrapper_callback callback,
							void *callback_data) {
	memset(sock_wrap, 0, sizeof(tcp_socket_wrapper));

	int result = 0;

	string_init(&sock_wrap->address);

	// figure out what address has been requested
	union {
		struct sockaddr addr;
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
	} addr;
	socklen_t addr_len;
	if (!address) {
		address = "::";
	}
	if (inet_pton(AF_INET, address, &addr.addr4.sin_addr) <= 0) {
		if (inet_pton(AF_INET6, address, &addr.addr6.sin6_addr) <= 0) {
			log_error("input address doesn't look like a valid IPv4 or IPv6 address: %s\n", address);
			result = 1;
			goto DONE;
		} else {
			addr.addr.sa_family = AF_INET6;
			addr.addr6.sin6_port = htons(port);
			addr_len = sizeof(struct sockaddr_in6);
		}
	} else {
		addr.addr.sa_family = AF_INET;
		addr.addr4.sin_port = htons(port);
		addr_len = sizeof(struct sockaddr_in);
	}

	// get a human-readable form of that bind address and port
	if (get_sockaddr_info_str((struct sockaddr *)&addr, &sock_wrap->address, &sock_wrap->port)) {
		log_error("failed to parse bound address and port for socket\n");
		result = 1;
		goto DONE;
	}
	log_trace("tcp_socket_wrapper_init %s:%i\n", string_get_cstr(&sock_wrap->address), sock_wrap->port);

	if (!callback) {
		log_error("tcp_socket_wrapper_init failed, callback is required\n");
		result = 1;
		goto DONE;
	}
	sock_wrap->callback = callback;
	sock_wrap->callback_data = callback_data;

	sock_wrap->socket = socket(addr.addr.sa_family, SOCK_STREAM, 0);
	if (sock_wrap->socket == -1) {
		log_error("tcp_socket_wrapper_init failed, error creating socket, %s\n", strerror(errno));
		result = 1;
		goto DONE;
	}
	int option = 1;
	if (setsockopt(sock_wrap->socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) {
		log_error("tcp_socket_wrapper_init failed, failed to set socket options %s\n", strerror(errno));
		result = 1;
		goto DONE;
	}
	if (bind(sock_wrap->socket, &addr.addr, addr_len) < 0) {
		log_error("tcp_socket_wrapper_init failed, failed to bind socket to port %i, %s\n", port, strerror(errno));
		result = 1;
		goto DONE;
	}
	// 2nd arg is number of connections that can be blocked waiting for the next accept
	if (listen(sock_wrap->socket, 10) < 0) {
		log_error("tcp_socket_wrapper_init failed, failed to listen on socket, %s\n", strerror(errno));
		result = 1;
		goto DONE;
	}

	sock_wrap->running = 1;
	if (pthread_create(&sock_wrap->thread, NULL, tcp_socket_wrapper_thread, sock_wrap)) {
		log_error("tcp_socket_wrapper_init failed, failed to make thread\n");
		result = 1;
		goto DONE;
	}
	sock_wrap->thread_is_init = 1;

	log_trace("tcp_socket_wrapper_init success\n");
DONE:
	if (result) {
		if (sock_wrap->socket) {
			close(sock_wrap->socket);
			sock_wrap->socket = 0;
		}
		string_dealloc(&sock_wrap->address);
	}
	return result;
}

int tcp_socket_wrapper_dealloc(tcp_socket_wrapper *sock_wrap) {
	log_trace("tcp_socket_wrapper_dealloc start\n");
	sock_wrap->running = 0;
	void *result;
	if (sock_wrap->thread_is_init && pthread_join(sock_wrap->thread, &result)) {
		log_error("tcp_socket_wrapper_dealloc failed, pthread_join failed during shutdown\n");
	}
	if (sock_wrap->socket) {
		close(sock_wrap->socket);
		sock_wrap->socket = 0;
	}
	string_dealloc(&sock_wrap->address);
	log_trace("tcp_socket_wrapper_dealloc success\n");
	return 0;
}

string *tcp_socket_wrapper_get_address(tcp_socket_wrapper *sock_wrap) {
	return &sock_wrap->address;
}

uint16_t tcp_socket_wrapper_get_port(tcp_socket_wrapper *sock_wrap) {
	return sock_wrap->port;
}