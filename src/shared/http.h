#ifndef http_h
#define http_h

#include "stream.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	// TODO body of request should be a stream, not fetch all data up front
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
 * Reads data from the given source until a complete HTTP document is parsed. Aborts when it's obvious that the document is malformed.
 * @param stream the data source to read from
 * @returns 0 when successful, non-0 when any error occurs reading from the file descriptor or if the content is malformed
 */
int http_request_parse(http_request *request, stream *stream);

/**
 * Writes the given status and headers to the destination stream
 * @param dst where to write the response to
 * @param status_code the HTTP status code
 * @param headers optional, the headers to include in the response
 * @returns 0 when successful, non-0 for failure
 */
int http_response_write(stream *dst, int status_code, http_headers *headers);
/**
 * As http_response_write, but also writes the given body if provided. If a body is provided, the Content-Length header is updated to match
 * the length.
 * @param dst where to write the response to
 * @param status_code the HTTP status code
 * @param headers optional, the headers to include in the response
 * @param body optional, the body to include in the response
 * @param body_len if body is provided must be set to the length of the body
 * @returns 0 when successful, non-0 for failure
 */
int http_response_write_data(stream *dst, int status_code, http_headers *headers, void *body, size_t body_len);
/**
 * As http_response_write_data, but provide the body as a buffer.
 */
int http_response_write_buffer(stream *dst, int status_code, http_headers *headers, buffer *body);
/**
 * As http_response_write_data, but provide the body as a string.
 */
int http_response_write_str(stream *dst, int status_code, http_headers *headers, string *body);
/**
 * As http_response_write_data, but provide the body as a 0-terminated c string.
 */
int http_response_write_cstr(stream *dst, int status_code, http_headers *headers, char *body);
/**
 * As http_response_write_data, but provide the body as a stream. Available bytes from the stream are read into temporary buffer first so
 * the full length can be calculated.
 */
int http_response_write_stream(stream *dst, int status_code, http_headers *headers, stream *body);

#ifdef __cplusplus
}
#endif

#endif
