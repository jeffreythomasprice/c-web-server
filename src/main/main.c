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
	size_t valuesLen;
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
	header->valuesLen = 0;
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
			header->valuesLen++;
			header->values = realloc(header->values, sizeof(char *) * header->valuesLen);
			size_t thisValueLen = i - last;
			char **thisValue = &header->values[header->valuesLen - 1];
			*thisValue = malloc(thisValueLen + 1);
			memcpy(*thisValue, values + last, thisValueLen);
			(*thisValue)[thisValueLen] = 0;

			// skip the comma
			i++;
			last = i;
		}
	}
	if (i > last) {
		header->valuesLen++;
		header->values = realloc(header->values, sizeof(char *) * header->valuesLen);
		size_t thisValueLen = i - last;
		char **thisValue = &header->values[header->valuesLen - 1];
		*thisValue = malloc(thisValueLen + 1);
		memcpy(*thisValue, values + last, thisValueLen);
		(*thisValue)[thisValueLen] = 0;
	}
}

void http_header_dealloc(http_header *header) {
	free(header->name);
	if (header->valuesLen > 0) {
		for (size_t i = 0; i < header->valuesLen; i++) {
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

	size_t bufferCapacity = CHUNK_READ_SIZE;
	uint8_t *buffer = malloc(bufferCapacity);
	size_t bufferLength = 0;

	// going to be looking for the end of the headers block
	// headers are separated from any body by an empty line, so we can look for a pair of new lines in a row
	size_t lastChecked = 0;
	size_t endOfRequestLine = -1;
	size_t endOfHeader = -1;

	// use max size -1 because we're going to want to put a terminating 0
	while (bufferLength < MAX_SOCKET_READ_SIZE_IN_CHUNKS * CHUNK_READ_SIZE - 1) {
		size_t result = read(data->socket, buffer + bufferLength, bufferCapacity - bufferLength);
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
		bufferLength += result;
		if (bufferCapacity - bufferLength < CHUNK_READ_SIZE) {
			bufferCapacity += CHUNK_READ_SIZE;
			buffer = realloc(buffer, bufferCapacity);
		}

		// checking for various blocks
		// everything before the body must use CRLF as the new line separator
		// if we haven't found the end of the request line check this chunk for a line break
		if (endOfRequestLine == -1 && bufferLength >= 2 && bufferLength - 2 >= lastChecked) {
			for (size_t i = lastChecked; i < bufferLength - 2; i++) {
				if (!memcmp(buffer + i, "\r\n", 2)) {
					endOfRequestLine = i;
					break;
				}
			}
			lastChecked = bufferLength - 2;
		}
		// if we haven't found the end of the headers check this chunk for an empty line
		if (endOfHeader == -1 && bufferLength >= 4 && bufferLength - 4 >= lastChecked) {
			// TODO JEFF header detection not working for GET requests with no body, missing last newline?
			for (size_t i = lastChecked; i < bufferLength - 4; i++) {
				if (!memcmp(buffer + i, "\r\n\r\n", 4)) {
					endOfHeader = i;

					// parse the headers
					http_headers headers;
					http_headers_init(&headers);
					size_t startOfLine = endOfRequestLine + 2;
					for (size_t endOfLine = startOfLine; endOfLine < endOfHeader + 2; endOfLine++) {
						if (!memcmp(buffer + endOfLine, "\r\n", 2)) {
							// TODO JEFF some of this is debug only
							size_t lineLen = endOfLine - startOfLine;
							char *line = malloc(lineLen + 1);
							memcpy(line, buffer + startOfLine, lineLen);
							line[lineLen] = 0;
							free(line);

							size_t colon;
							for (colon = startOfLine; colon < endOfLine; colon++) {
								if (buffer[colon] == ':') {
									break;
								}
							}
							if (colon == endOfLine) {
								log_error("TODO JEFF handle the fact that this line isn't a valid header, no value: %s\n", line);
							} else {
								http_header *header = http_headers_get_or_create(&headers, buffer + startOfLine, colon - startOfLine);
								size_t valueLen = endOfLine - colon - 1;
								size_t startOfValue = colon + 1;
								if (buffer[startOfValue] == ' ') {
									valueLen--;
									startOfValue++;
								}
								http_header_append(header, buffer + startOfValue, valueLen);
								log_trace("TODO JEFF parsed header, name: %s\n", header->name);
								for (size_t j = 0; j < header->valuesLen; j++) {
									log_trace("TODO JEFF value[%llu] = %s\n", j, header->values[j]);
								}
							}

							// skip the new line
							endOfLine += 2;
							startOfLine = endOfLine;
						}
					}

					// TODO JEFF debugging
					for (size_t headerIndex = 0; headerIndex < headers.len; headerIndex++) {
						http_header *header = &headers.headers[headerIndex];
						log_trace("TODO JEFF headers[%llu], name: %s\n", headerIndex, header->name);
						for (size_t valueIndex = 0; valueIndex < header->valuesLen; valueIndex++) {
							log_trace("TODO JEFF     value[%llu] = %s\n", valueIndex, header->values[valueIndex]);
						}
					}

					/*
					TODO JEFF handle content length
					if no content length provided then we're done
					if provided, and the total message length would be more than our max read size, abort
					otherwise set the next read to be the remainder of the message
					*/

					http_headers_dealloc(&headers);

					break;
				}
			}
			lastChecked = bufferLength - 4;
		}
	}
	buffer[bufferLength] = 0;

	if (endOfRequestLine != -1) {
		char *s = malloc(endOfRequestLine + 1);
		memcpy(s, buffer, endOfRequestLine);
		s[endOfRequestLine] = 0;
		log_trace("TODO JEFF request line:\n%s\n", s);
		free(s);
	}
	if (endOfHeader != -1) {
		// skip the new line at the end of the request line
		size_t startOfHeaders = endOfRequestLine + 2;
		size_t headerSize = endOfHeader - startOfHeaders;
		char *s = malloc(headerSize + 1);
		memcpy(s, buffer + startOfHeaders, headerSize);
		s[headerSize] = 0;
		log_trace("TODO JEFF headers:\n%s\n", s);
		free(s);
	}

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