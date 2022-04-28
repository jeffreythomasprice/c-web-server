#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "http.h"
#include "log.h"

// TODO use a reasonable chunk size
#define CHUNK_READ_SIZE 50
#define MAX_SOCKET_READ_SIZE_IN_CHUNKS 64

/**
 * Initialize the given header struct with the given name. The values are initialized to an empty list.
 *
 * The length of the name is given by nameLen (not including a terminating 0).
 */
void http_header_init(http_header *header, char *name, size_t name_len) {
	header->name = malloc(name_len + 1);
	memcpy(header->name, name, name_len);
	header->name[name_len] = 0;
	header->values_len = 0;
	header->values = NULL;
}

/**
 * Appends the given values to this header. The values may be a comma-delimited list which are split into multiple values.
 *
 * The length of the values string is given by valuesLen (not including a terminating 0).
 */
void http_header_append(http_header *header, char *values, size_t values_len) {
	size_t last = 0;
	size_t i;
	for (i = 0; i < values_len; i++) {
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

http_header *http_headers_get_or_create(http_headers *headers, char *name, size_t name_len) {
	// TODO JEFF should be case-insensitive
	for (size_t i = 0; i < headers->len; i++) {
		size_t len = strlen(headers->headers[i].name);
		if (name_len == len && !memcmp(headers->headers[i].name, name, len)) {
			return &headers->headers[i];
		}
	}
	headers->len++;
	headers->headers = realloc(headers->headers, sizeof(http_header) * headers->len);
	http_header_init(&headers->headers[headers->len - 1], name, name_len);
	return &headers->headers[headers->len - 1];
}

void http_request_init(http_request *request, char *method, size_t method_len, char *uri, size_t uri_len) {
	request->method = malloc(method_len + 1);
	memcpy(request->method, method, method_len);
	request->method[method_len] = 0;
	request->uri = malloc(uri_len + 1);
	memcpy(request->uri, uri, uri_len);
	request->uri[uri_len] = 0;
	http_headers_init(&request->headers);
}

int http_request_init_from_file(http_request *request, int fd) {
	size_t buffer_capacity = CHUNK_READ_SIZE;
	uint8_t *buffer = malloc(buffer_capacity);
	size_t buffer_length = 0;

	size_t end_of_last_line = 0;
	int found_request_line = 0;
	size_t end_of_request_line;
	int found_end_of_header = 0;
	size_t start_of_body;

	int is_failed = 0;
	int is_request_init = 0;

	// use max size -1 because we're going to want to put a terminating 0
	size_t max_read_length = MAX_SOCKET_READ_SIZE_IN_CHUNKS * CHUNK_READ_SIZE;
	while (buffer_length < max_read_length) {
		log_trace("TODO JEFF about to read, buffer_length: %llu, max_read_length: %llu\n", buffer_length, max_read_length);
		size_t result = read(fd, buffer + buffer_length, buffer_capacity - buffer_length);
		log_trace("TODO JEFF read result: %llu\n", result);
		if (result < 0) {
			log_error("http_task failed, error reading from socket %s\n", strerror(errno));
			is_failed = 1;
			goto DONE;
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
					size_t line_length = i - end_of_last_line;
					if (!found_request_line) {
						found_request_line = 1;
						end_of_request_line = i;
						int found_start_of_method = 0;
						int found_end_of_method = 0;
						size_t method_start, method_end;
						int found_start_of_uri = 0;
						int found_end_of_uri = 0;
						size_t uri_start, uri_end;
						int found_start_of_protocol_version = 0;
						int found_end_of_protocol_version = 0;
						size_t protocol_version_start, protocol_version_end;
						for (size_t i = 0; i < end_of_request_line; i++) {
							int is_space = buffer[i] == ' ';
							if (!found_start_of_method) {
								if (!is_space) {
									found_start_of_method = 1;
									method_start = i;
								}
							} else if (!found_end_of_method) {
								if (is_space) {
									found_end_of_method = 1;
									method_end = i;
								}
							} else if (!found_start_of_uri) {
								if (!is_space) {
									found_start_of_uri = 1;
									uri_start = i;
								}
							} else if (!found_end_of_uri) {
								if (is_space) {
									found_end_of_uri = 1;
									uri_end = i;
								}
							} else if (!found_start_of_protocol_version) {
								if (!is_space) {
									found_start_of_protocol_version = 1;
									protocol_version_start = i;
								}
							} else if (!found_end_of_protocol_version) {
								if (is_space) {
									found_end_of_protocol_version = 1;
									protocol_version_end = i;
									break;
								} else if (i == end_of_request_line - 1) {
									found_end_of_protocol_version = 1;
									protocol_version_end = i + 1;
									break;
								}
							}
						}
						if (found_end_of_protocol_version) {
							is_request_init = 1;
							http_request_init(request, buffer + method_start, method_end - method_start, buffer + uri_start,
											  uri_end - uri_start);
						} else {
							char *request_line = malloc(end_of_request_line + 1);
							memcpy(request_line, buffer, end_of_request_line);
							request_line[end_of_request_line] = 0;
							log_error("failed to parse request line: %s\n", request_line);
							free(request_line);
							is_failed = 1;
							goto DONE;
						}
					} else if (is_request_init && !found_end_of_header) {
						if (line_length == 0) {
							found_end_of_header = 1;
							start_of_body = i + 2;
							// at this point we either have a content length header or we don't
							// either way we know exactly how many more bytes to read, which may be 0
							http_header *header = http_headers_get_or_create(&request->headers, "Content-Length", 14);
							if (header->values_len == 1) {
								int content_length;
								if (sscanf(header->values[0], "%i", &content_length) == 1) {
									max_read_length = i + content_length;
								} else {
									log_error("Content-Length header present but isn't a valid integer: %s\n", header->values[0]);
									is_failed = 1;
									goto DONE;
								}
							} else {
								max_read_length = buffer_length;
							}
							break;
						} else {
							size_t colonIndex = 0;
							int found_separator = 0;
							for (colonIndex = 0; colonIndex < line_length; colonIndex++) {
								// TODO should strip off leading spaces after the colon too
								if (buffer[end_of_last_line + colonIndex] == ':') {
									found_separator = 1;
									break;
								}
							}
							if (found_separator) {
								http_header *header = http_headers_get_or_create(&request->headers, buffer + end_of_last_line, colonIndex);
								http_header_append(header, buffer + end_of_last_line + colonIndex + 1, line_length - colonIndex - 1);
							} else {
								char *line = malloc(line_length + 1);
								memcpy(line, buffer + end_of_last_line, line_length);
								line[line_length] = 0;
								log_error("failed to parse header line: %s\n", line);
								free(line);
								is_failed = 1;
								goto DONE;
							}
						}
					}
					// skip the newline
					end_of_last_line = i + 2;
					i++;
				}
			}
		}
	}

	if (is_request_init) {
		request->body_len = buffer_length - start_of_body;
		if (request->body_len == 0) {
			request->body = NULL;
		} else {
			request->body = malloc(request->body_len + 1);
			memcpy(request->body, buffer + start_of_body, request->body_len);
			request->body[request->body_len] = 0;
		}

		log_trace("TODO JEFF method: %s\n", request->method);
		log_trace("TODO JEFF uri: %s\n", request->uri);

		log_trace("TODO JEFF headers len: %lu\n", request->headers.len);
		for (size_t i = 0; i < request->headers.len; i++) {
			log_trace("TODO JEFF header[%lu] = %s\n", i, request->headers.headers[i].name);
			for (size_t j = 0; j < request->headers.headers[i].values_len; j++) {
				log_trace("TODO JEFF     value[%lu] = %s\n", j, request->headers.headers[i].values[j]);
			}
		}

		if (request->body > 0) {
			log_trace("TODO JEFF body len: %lu\nbody:\n%s\n", request->body_len, request->body);
		} else {
			log_trace("TODO JEFF no body\n");
		}
	}

DONE:
	free(buffer);
	if (is_failed) {
		if (!is_request_init) {
			http_request_dealloc(request);
		}
		return 1;
	}
	return 0;
}

void http_request_dealloc(http_request *request) {
	free(request->method);
	free(request->uri);
	http_headers_dealloc(&request->headers);
	if (request->body) {
		free(request->body);
	}
}
