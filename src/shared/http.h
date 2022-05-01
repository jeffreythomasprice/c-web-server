#ifndef http_h
#define http_h

#include "string.h"

// TODO needs tests

typedef struct {
	string name;
	size_t values_capacity;
	size_t values_length;
	string *values;
} http_header;

typedef struct {
	size_t headers_capacity;
	size_t headers_length;
	http_header *headers;
} http_headers;

typedef struct {
	buffer read_buf;
	string scratch;
	string method;
	string uri;
	http_headers headers;
	buffer body;
} http_request;

void http_header_init(http_header *header);
void http_header_dealloc(http_header *header);
void http_header_clear(http_header *header);
string *http_header_get_name(http_header *header);
size_t http_header_get_num_values(http_header *header);
string *http_header_get_value(http_header *header, size_t i);
string *http_header_append_value(http_header *header);

void http_headers_init(http_headers *headers);
void http_headers_dealloc(http_headers *headers);
size_t http_headers_get_num(http_headers *headers);
http_header *http_headers_get(http_headers *headers, size_t i);
void http_headers_clear(http_headers *headers);
http_header *http_headers_get_cstr(http_headers *headers, char *name, int create_if_missing);
http_header *http_headers_get_cstr_len(http_headers *headers, char *name, size_t name_len, int create_if_missing);

void http_request_init(http_request *request);
void http_request_dealloc(http_request *request);
/**
 * Resets the state of this request to contain the data from the given file descriptor.
 * @param fd a file descriptor (probably a socket)
 * @returns 0 when successful, non-0 when any error occurs reading from the file descriptor or if the content is malformed
 */
int http_request_clear_and_read_from_file(http_request *request, int fd);

#endif
