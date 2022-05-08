#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "../shared/http.h"
#include "../shared/log.h"

#define DEFAULT_PORT 8000
#define DEFAULT_WORKER_POOL_SIZE 2
#define DEFAULT_WORKER_POOL_QUEUE_SIZE 10
// 5 seconds in nanoseconds
#define DEFAULT_HTTP_TIMEOUT 5000000000llu

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

int handle_request(void *data, http_request *request, http_response *response) {
	// TODO some real HTTP request-response stuff
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

	http_server server;
	if (http_server_init(&server, handle_request, NULL, NULL, port, DEFAULT_WORKER_POOL_SIZE, DEFAULT_WORKER_POOL_QUEUE_SIZE,
						 DEFAULT_HTTP_TIMEOUT)) {
		log_error("failed to make HTTP server\n");
		return 1;
	}

	shutdown_requested = 0;
	signal(SIGINT, signal_handler);
	log_debug("waiting for requests on port %i\n", port);
	// TODO should be a semaphore that is signalled when we're shutting down
	while (!shutdown_requested) {
		// 1 second in microseconds
		usleep(1000000llu);
	}

	if (http_server_dealloc(&server)) {
		log_error("failed to clean up HTTP server\n");
		return 1;
	}

	return 0;
}