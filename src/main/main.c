#include <errno.h>
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

// TODO use a reasonable chunk size
#define CHUNK_READ_SIZE 50
#define MAX_SOCKET_READ_SIZE_IN_CHUNKS 64

typedef struct {
	int socket;
} http_worker_task_data;

// TODO break headers out into their own file
typedef struct {
	char *name;
	size_t values_len;
	char **values;
} http_header;

typedef struct {
	size_t len;
	http_header *headers;
} http_headers;

/**
 * Initialize the given header struct with the given name. The values are initialized to an empty list.
 *
 * The length of the name is given by nameLen (not including a terminating 0).
 */
void http_header_init(http_header *header, char *name, size_t nameLen) {
	header->name = malloc(nameLen + 1);
	memcpy(header->name, name, nameLen);
	header->name[nameLen] = 0;
	header->values_len = 0;
	header->values = NULL;
}

/**
 * Appends the given values to this header. The values may be a comma-delimited list which are split into multiple values.
 *
 * The length of the values string is given by valuesLen (not including a terminating 0).
 */
void http_header_append(http_header *header, char *values, size_t valuesLen) {
	size_t last = 0;
	size_t i;
	for (i = 0; i < valuesLen; i++) {
		if (values[i] == ',') {
			header->values_len++;
			header->values = realloc(header->values, sizeof(char *) * header->values_len);
			size_t thisValueLen = i - last;
			char **thisValue = &header->values[header->values_len - 1];
			*thisValue = malloc(thisValueLen + 1);
			memcpy(*thisValue, values + last, thisValueLen);
			(*thisValue)[thisValueLen] = 0;

			// skip the comma
			i++;
			last = i;
		}
	}
	if (i > last) {
		header->values_len++;
		header->values = realloc(header->values, sizeof(char *) * header->values_len);
		size_t thisValueLen = i - last;
		char **thisValue = &header->values[header->values_len - 1];
		*thisValue = malloc(thisValueLen + 1);
		memcpy(*thisValue, values + last, thisValueLen);
		(*thisValue)[thisValueLen] = 0;
	}
}

void http_header_dealloc(http_header *header) {
	free(header->name);
	if (header->values_len > 0) {
		for (size_t i = 0; i < header->values_len; i++) {
			free(header->values[i]);
		}
		free(header->values);
	}
}

void http_headers_init(http_headers *headers) {
	headers->len = 0;
	headers->headers = NULL;
}

void http_headers_dealloc(http_headers *headers) {
	if (headers->headers > 0) {
		for (size_t i = 0; i < headers->len; i++) {
			http_header_dealloc(&headers->headers[i]);
		}
		free(headers->headers);
	}
}

http_header *http_headers_get_or_create(http_headers *headers, char *name, size_t nameLen) {
	for (size_t i = 0; i < headers->len; i++) {
		size_t len = strlen(headers->headers[i].name);
		if (nameLen == len && !memcmp(headers->headers[i].name, name, len)) {
			return &headers->headers[i];
		}
	}
	headers->len++;
	headers->headers = realloc(headers->headers, sizeof(http_header) * headers->len);
	http_header_init(&headers->headers[headers->len - 1], name, nameLen);
	return &headers->headers[headers->len - 1];
}

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

	size_t buffer_capacity = CHUNK_READ_SIZE;
	uint8_t *buffer = malloc(buffer_capacity);
	size_t buffer_length = 0;

	size_t end_of_last_line = 0;
	int found_request_line = 0;
	int found_end_of_header = 0;
	http_headers headers;
	http_headers_init(&headers);
	size_t start_of_body;

	// use max size -1 because we're going to want to put a terminating 0
	while (buffer_length < MAX_SOCKET_READ_SIZE_IN_CHUNKS * CHUNK_READ_SIZE - 1) {
		size_t result = read(data->socket, buffer + buffer_length, buffer_capacity - buffer_length);
		if (result < 0) {
			log_error("http_task failed, error reading from socket %s\n", strerror(errno));
			// TODO respond to socket with a failure
			free(buffer);
			close(data->socket);
			return 1;
		}
		if (result == 0) {
			break;
		}
		buffer_length += result;
		if (buffer_capacity - buffer_length < CHUNK_READ_SIZE) {
			buffer_capacity += CHUNK_READ_SIZE;
			buffer = realloc(buffer, buffer_capacity);
		}

		if (!found_request_line || !found_end_of_header) {
			for (size_t i = end_of_last_line; i < buffer_length - 1; i++) {
				// rfc says all lines before the body start with CRLF
				if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
					size_t lineLength = i - end_of_last_line;
					// TODO JEFF line is for easy debugging
					char *line = malloc(lineLength + 1);
					memcpy(line, buffer + end_of_last_line, lineLength);
					line[lineLength] = 0;
					if (!found_request_line) {
						found_request_line = 1;
						printf("TODO JEFF parse request line: %s\n", line);
					} else if (!found_end_of_header) {
						if (lineLength == 0) {
							found_end_of_header = 1;
							start_of_body = i + 2;
						} else {
							size_t colonIndex = 0;
							int foundSeparator = 0;
							for (colonIndex = 0; colonIndex < lineLength; colonIndex++) {
								if (buffer[end_of_last_line + colonIndex] == ':') {
									foundSeparator = 1;
									break;
								}
							}
							if (foundSeparator) {
								http_header *header = http_headers_get_or_create(&headers, buffer + end_of_last_line, colonIndex);
								http_header_append(header, buffer + end_of_last_line + colonIndex + 1, lineLength - colonIndex - 1);
							} else {
								log_error("TODO JEFF handle error parsing header: %s\n", line);
							}
						}
					}
					free(line);
					// skip the newline
					end_of_last_line = i + 2;
					i++;
				}
			}
		}
	}
	buffer[buffer_length] = 0;

	log_trace("TODO JEFF headers len: %lu\n", headers.len);
	for (size_t i = 0; i < headers.len; i++) {
		log_trace("TODO JEFF header[%lu] = %s\n", i, headers.headers[i].name);
		for (size_t j = 0; j < headers.headers[i].values_len; j++) {
			log_trace("TODO JEFF     value[%lu] = %s\n", j, headers.headers[i].values[j]);
		}
	}

	size_t body_length = buffer_length - start_of_body;
	if (body_length > 0) {
		// TODO body is for debugging
		char *body = malloc(body_length + 1);
		memcpy(body, buffer + start_of_body, body_length);
		body[body_length] = 0;
		log_trace("TODO JEFF body len: %lu\nbody:\n%s\n", body_length, body);
		free(body);
	} else {
		log_trace("TODO JEFF no body\n");
	}

	http_headers_dealloc(&headers);
	free(buffer);
	close(data->socket);
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