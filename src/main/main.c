#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../shared/log.h"
#include "../shared/worker_thread_pool.h"

#define DEFAULT_PORT 8000
#define DEFAULT_WORKER_POOL_SIZE 2

typedef struct {
	int socket;
} http_worker_task_data;

int shutdown_requested;

void usage(char *name) {
	printf("usage:\n");
	printf("    %s [options]\n", name);
	printf("    -h, --help\n");
	printf("        display help text\n");
	printf("    -p, --port PORT\n");
	printf("        Listen on this port\n");
}

void signal_handler(int signum) {
	switch (signum) {
	case SIGINT:
		log_debug("stop requested\n");
		shutdown_requested = 1;
		break;
	default:
		log_error("unrecognized signal %i\n", signum);
	}
}

int http_worker_task(void *data) {
	http_worker_task_data *d = data;
	log_debug("TODO JEFF http_worker_task\n");
	return 0;
}

int main(int argc, char **argv) {
	// parsing options

	int port = DEFAULT_PORT;

	// suppress getopt logging
	opterr = 0;
	while (1) {
		static struct option arg_options[] = {{"help", 0, 0, 0}, {"port", required_argument, 0, 0}};
		int option_index = 0;

		int c = getopt_long(argc, argv, "hp:", arg_options, &option_index);
		if (c == -1) {
			break;
		}
		if ((c == 0 && option_index == 0) || c == 'h') {
			usage(argv[0]);
			return 0;
		}
		if ((c == 0 && option_index == 1) || c == 'p') {
			if (!optarg) {
				return 1;
			}
			if (sscanf(optarg, "%i", &port) != 1) {
				log_error("failed to parse port: %s\n", optarg);
				return 1;
			}
			continue;
		}
		log_error("unrecognized arg: %s\n", argv[optind - 1]);
		usage(argv[0]);
		return 1;
	}
	if (optind < argc) {
		log_error("unrecognized arg: %s\n", argv[optind]);
		usage(argv[0]);
		return 1;
	}

	int return_value = 0;

	int socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	int socket_needs_close = 0;
	if (!socket_handle) {
		log_error("error creating socket\n");
		return_value = 1;
		goto DONE;
	}
	socket_needs_close = 1;

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if (bind(socket_handle, (struct sockaddr *)&address, sizeof(address)) < 0) {
		// TODO check errno
		log_error("failed to bind socket to port %i\n", port);
		return_value = 1;
		goto DONE;
	}

	// TODO 2nd arg is how many pending connections should be allowed to be queued here, what's a good number?
	if (listen(socket_handle, 0) < 0) {
		// TODO check errno
		log_error("failed to listen on socket\n");
		return_value = 1;
		goto DONE;
	}

	// start worker thread pool
	worker_thread_pool http_worker_pool;
	int http_worker_pool_needs_destroy = 0;
	if (worker_thread_pool_init(&http_worker_pool, 1, 10)) {
		log_error("failed to make thread pool\n");
		return_value = 1;
		goto DONE;
	}
	http_worker_pool_needs_destroy = 1;

	shutdown_requested = 0;
	signal(SIGINT, signal_handler);
	log_debug("waiting for requests on port %i\n", port);
	while (!shutdown_requested) {
		// accept new incoming connection
		struct sockaddr_in request_address;
		socklen_t request_address_len = sizeof(struct sockaddr_in);
		// TODO JEFF figure out how to do timeout on accept, otherwise we can't signal this
		int request_socket = accept(socket_handle, (struct sockaddr *)&request_address, &request_address_len);

		// some basic error checking
		if (request_socket == -1) {
			log_error("accepting incoming connection failed with %i\n", errno);
			continue;
		}
		if (request_address.sin_family == AF_INET6) {
			log_error("IPv6 unsupported\n");
			close(request_socket);
			continue;
		}
		if (request_address.sin_family != AF_INET) {
			log_error("unrecognized address family %i\n", request_address.sin_family);
			close(request_socket);
			continue;
		}

		char request_address_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &request_address.sin_addr, request_address_str, INET_ADDRSTRLEN);
		log_trace("incoming request from %s:%i\n", request_address_str, ntohs(request_address.sin_port));

		// TODO JEFF should be launching new task
		close(request_socket);

		// TODO JEFF for testing purposes only
		log_debug("TODO JEFF main loop...\n");
		usleep(1000000llu);
	}

DONE:
	if (http_worker_pool_needs_destroy) {
		worker_thread_pool_destroy(&http_worker_pool);
	}
	close(socket_handle);
	return return_value;
}