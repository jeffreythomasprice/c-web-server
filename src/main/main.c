#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../shared/log.h"
#include "../shared/tcp_socket_wrapper.h"
#include "../shared/worker_thread_pool.h"

#define DEFAULT_PORT 8000
#define DEFAULT_WORKER_POOL_SIZE 2

typedef struct {
	int socket;
} http_worker_task_data;

int shutdown_requested;
worker_thread_pool http_worker_pool;

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

int http_task(int id, void *d) {
	http_worker_task_data *data = d;

	log_trace("TODO JEFF read and handle http request\n");
	const size_t bufferLen = 1024;
	uint8_t *buffer = malloc(bufferLen);
	memset(buffer, 0, bufferLen);
	size_t len = read(data->socket, buffer, bufferLen);
	log_debug("TODO JEFF read some data from the request, treating as ascii:\n%s\n", buffer);
	free(buffer);

	close(data->socket);
	free(data);
	return 0;
}

void socket_accept(int s) {
	// 5 seconds in nanoseconds
	const uint64_t timeout = 5000000000llu;

	http_worker_task_data *data = malloc(sizeof(http_worker_task_data));
	data->socket = s;
	int enqueue_error = worker_thread_pool_enqueue(&http_worker_pool, http_task, data, NULL, timeout);
	switch (enqueue_error) {
	case 0:
		// nothing to do, this is the success case
		// we can assume the worker thread will clean up everything
		return;
	case WORKER_THREAD_POOL_ERROR_QUEUE_FULL:
		log_error("failed to execute incoming request, queue is full\n");
		// we didn't enqueue, so we can't rely on them to free memory
		free(data);
		close(s);
		break;
	case WORKER_THREAD_POOL_ERROR_TIMEOUT:
		log_error("timed out waiting on request\n");
		// we did enqueue, it's just taking a long time
		// we'll close the socket but leave the memory alone so the other end can eventually free it
		// TODO send a timeout http response?
		close(s);
		break;
	default:
		log_error("failed to execute incoming request, %i\n", enqueue_error);
		// any other error we assume we didn't end up in the queue so clean up
		free(data);
		close(s);
		break;
	}
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

	// start worker thread pool
	if (worker_thread_pool_init(&http_worker_pool, 1, 10)) {
		log_error("failed to make thread pool\n");
		return 1;
	}

	tcp_socket_wrapper sock_wrap;
	if (tcp_socket_wrapper_init(&sock_wrap, NULL, port, socket_accept)) {
		log_error("failed to make socket\n");
		worker_thread_pool_destroy(&http_worker_pool);
		return 1;
	}

	shutdown_requested = 0;
	signal(SIGINT, signal_handler);
	log_debug("waiting for requests on port %i\n", port);
	// TODO should be a semaphore that is signalled when we're shutting down
	while (!shutdown_requested) {
		usleep(1000000llu);
	}

	// TODO log failures
	tcp_socket_wrapper_destroy(&sock_wrap);
	worker_thread_pool_destroy(&http_worker_pool);

	return 0;
}