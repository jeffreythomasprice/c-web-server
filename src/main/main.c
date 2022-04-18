#include <getopt.h>
#include <signal.h>
#include <stdio.h>
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
	worker_thread_pool http_worker_pool;
	if (worker_thread_pool_init(&http_worker_pool, 1, 10)) {
		log_error("failed to make thread pool\n");
		return 1;
	}

	tcp_socket_wrapper sock_wrap;
	if (tcp_socket_wrapper_init(&sock_wrap, NULL, port)) {
		log_error("failed to make socket\n");
		worker_thread_pool_destroy(&http_worker_pool);
		return 1;
	}

	shutdown_requested = 0;
	signal(SIGINT, signal_handler);
	log_debug("waiting for requests on port %i\n", port);
	while (!shutdown_requested) {
		// TODO JEFF for testing purposes only
		log_debug("TODO JEFF main loop...\n");
		usleep(1000000llu);
	}

	return 0;
}