#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8000
#define DEFAULT_WORKER_POOL_SIZE 2

#define ONE_SECOND_IN_MICROSECONDS 1000000

struct worker_thread_data {
	pthread_t thread;
	int id;
	int running;
};

void usage(char *name) {
	printf("usage:\n");
	printf("    %s [options]\n", name);
	printf("    -h, --help\n");
	printf("        display help text\n");
	printf("    -p, --port PORT\n");
	printf("        Listen on this port\n");
}

void *worker_thread(void *data) {
	struct worker_thread_data *thread_data = data;
	printf("TODO implement worker thread %i\n", thread_data->id);
	while (thread_data->running) {
		printf("TODO JEFF worker thread tick %i\n", thread_data->id);
		usleep(ONE_SECOND_IN_MICROSECONDS);
	}
	return NULL;
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
			exit(0);
		}
		if ((c == 0 && option_index == 1) || c == 'p') {
			if (!optarg) {
				exit(1);
			}
			if (sscanf(optarg, "%i", &port) != 1) {
				fprintf(stderr, "failed to parse port: %s\n", optarg);
				exit(1);
			}
			continue;
		}
		fprintf(stderr, "unrecognized arg: %s\n", argv[optind - 1]);
		usage(argv[0]);
		exit(1);
		break;
	}
	if (optind < argc) {
		fprintf(stderr, "unrecognized arg: %s\n", argv[optind]);
		usage(argv[0]);
		exit(1);
	}

	int socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if (!socket_handle) {
		fprintf(stderr, "error creating socket\n");
		return 1;
	}

	int worker_pool_size = DEFAULT_WORKER_POOL_SIZE;

	int return_value = 0;

	// start worker thread pool
	struct worker_thread_data *worker_threads = calloc(worker_pool_size, sizeof(struct worker_thread_data));
	for (int i = 0; i < worker_pool_size; i++) {
		worker_threads[i].id = i;
		worker_threads[i].running = 1;
		if (pthread_create(&worker_threads[i].thread, NULL, worker_thread, &worker_threads[i])) {
			// TODO JEFF log the return value of pthread_create, it's an error code
			fprintf(stderr, "failed to start worker threads\n");
			return_value = 1;
			goto DONE;
		}
	}

	// listen on socket

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if (bind(socket_handle, (struct sockaddr *)&address, sizeof(address)) < 0) {
		// TODO check errno
		fprintf(stderr, "failed to bind socket to port %i\n", port);
		return_value = 1;
		goto DONE;
	}

	// TODO 2nd arg is how many pending connections should be allowed to be queued here, what's a good number?
	if (listen(socket_handle, 0) < 0) {
		// TODO check errno
		fprintf(stderr, "failed to listen on socket");
		exit(1);
		return_value = 1;
		goto DONE;
	}

	printf("TODO listen\n");

DONE:
	for (int i = 0; i < worker_pool_size; i++) {
		worker_threads[i].running = 0;
		// TODO signal any locks or semaphores so thread can abort what it's doing or wake up
	}
	for (int i = 0; i < worker_pool_size; i++) {
		if (pthread_join(worker_threads[i].thread, NULL)) {
			fprintf(stderr, "error occurred waiting on worker thread\n");
		}
	}
	free(worker_threads);
	close(socket_handle);
	return return_value;
}