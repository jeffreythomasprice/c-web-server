/*
An implementation of an HTTP web server.

References:
https://www.ietf.org/rfc/rfc2616.txt
https://www.ietf.org/rfc/rfc7230.txt
*/

#ifndef http_h
#define http_h

#include "stream.h"
#include "string.h"
#include "tcp_socket_wrapper.h"
#include "worker_thread_pool.h"

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
	// TODO uri should be the uri type
	string uri;
	http_headers headers;
	// TODO body of request should be a stream, not fetch all data up front
	buffer body;
} http_request;

typedef struct {
	string scratch;
	int status_code;
	string reason_phrase;
	http_headers headers;
	buffer body_buffer;
	stream body_stream;
} http_response;

typedef int (*http_server_func)(void *data, http_request *request, http_response *response);

struct http_server_task_data;
struct http_server;

typedef struct http_server_task_data {
	struct http_server_task_data *next;
	struct http_server *server;
	string scratch;
	string request_address;
	uint16_t request_port;
	http_request request;
	http_response response;
	int socket;
	stream socket_stream;
} http_server_task_data;

typedef struct http_server {
	http_server_func callback;
	void *callback_data;
	uint64_t timeout;
	tcp_socket_wrapper socket;
	worker_thread_pool thread_pool;
	pthread_mutex_t task_pool_mutex;
	size_t task_pool_len;
	http_server_task_data *task_pool;
} http_server;

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
string *http_request_get_method(http_request *request);
string *http_request_get_uri(http_request *request);
http_headers *http_request_get_headers(http_request *request);
/**
 * Clears all data from this request ahead of time and replaces it with new data parsed from the input stream. Aborts when it's obvious that
 * the document is malformed.
 * @param stream the data source to read from
 * @returns 0 when successful, non-0 when any error occurs reading from the stream or if the content is malformed
 */
int http_request_parse(http_request *request, stream *stream);

void http_response_init(http_response *response);
void http_response_dealloc(http_response *response);
void http_response_clear(http_response *response);
int http_response_get_status_code(http_response *response);
void http_response_set_status_code(http_response *response, int status_code);
string *http_response_get_reason_phrase(http_response *response);
http_headers *http_response_get_headers(http_response *response);
stream *http_response_get_body(http_response *response);
/**
 * Serializes this response to the destination stream.
 * @param stream the stream to write to
 * @returns 0 when successful, non-0 when any error occurs writing to the stream
 */
int http_response_write(http_response *response, stream *stream);

/**
 * Maintains a socket that it accepts incoming HTTP requests on. It invokes the given callback in a thread pool, and then responds with the
 * filled in response. Task failures or timeouts generate default responses.
 * @param address passed to tcp_socket_wrapper_init
 * @param port passed to tcp_socket_wrapper_init
 * @param num_threads passed to worker_thread_pool_init
 * @param queue_size passed to worker_thread_pool_init
 * @param timeout the time wait on the server callback to return before sending back an error response, in nanoseconds
 * @return 0 when successful, non-0 when an error occurs or on bad arguments
 */
int http_server_init(http_server *server, http_server_func callback, void *callback_data, char *address, uint16_t port, int num_threads,
					 int queue_size, uint64_t timeout);
int http_server_dealloc(http_server *server);

#ifdef __cplusplus
}
#endif

#endif
