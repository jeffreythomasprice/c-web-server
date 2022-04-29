#ifndef http_h
#define http_h

#include <stdint.h>
#include <stdlib.h>

// TODO needs tests

// TODO JEFF everything in here should be using buffer and string

typedef struct {
	char *name;
	size_t values_len;
	char **values;
} http_header;

typedef struct {
	size_t len;
	http_header *headers;
} http_headers;

typedef struct {
	char *method;
	char *uri;
	http_headers headers;
	size_t body_len;
	uint8_t *body;
} http_request;

/**
 * Initialize the given header struct with the given name. The values are initialized to an empty list.
 *
 * The length of the name is given by nameLen (not including a terminating 0).
 */
void http_header_init(http_header *header, char *name, size_t name_len);
/**
 * Appends the given values to this header. The values may be a comma-delimited list which are split into multiple values.
 *
 * The length of the values string is given by valuesLen (not including a terminating 0).
 */
void http_header_append(http_header *header, char *values, size_t values_len);
void http_header_dealloc(http_header *header);

void http_headers_init(http_headers *headers);
void http_headers_dealloc(http_headers *headers);
http_header *http_headers_get_or_create(http_headers *headers, char *name, size_t name_len);

/**
 * @param fd a file descriptor (probably a socket)
 * @returns 0 when successful, non-0 when any error occurs reading from the file descriptor or if the content is malformed
 */
int http_request_init_from_file(http_request *request, int fd);
void http_request_dealloc(http_request *request);

#endif
