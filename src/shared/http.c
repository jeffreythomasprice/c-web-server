#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "log.h"

#define CHUNK_READ_SIZE 1024
#define MAX_SOCKET_READ_SIZE_IN_CHUNKS 64

void http_header_init(http_header *header) {
	string_init(&header->name);
	header->values_capacity = 0;
	header->values_length = 0;
	header->values = NULL;
}

void http_header_dealloc(http_header *header) {
	string_dealloc(&header->name);
	if (header->values_capacity > 0) {
		for (size_t i = 0; i < header->values_capacity; i++) {
			string_dealloc(&header->values[i]);
		}
		free(header->values);
	}
}

void http_header_clear(http_header *header) {
	header->values_length = 0;
}

string *http_header_get_name(http_header *header) {
	return &header->name;
}

size_t http_header_get_num_values(http_header *header) {
	return header->values_length;
}

string *http_header_get_value(http_header *header, size_t i) {
	if (i >= header->values_length) {
		return NULL;
	}
	return &header->values[i];
}

string *http_header_append_value(http_header *header) {
	if (header->values_length == header->values_capacity) {
		if (header->values_capacity == 0) {
			header->values = malloc(sizeof(string));
		} else {
			header->values = realloc(header->values, sizeof(string) * (header->values_capacity + 1));
		}
		string_init(&header->values[header->values_capacity]);
		header->values_capacity++;
	}
	string *result = &header->values[header->values_length];
	header->values_length++;
	return result;
}

void http_headers_init(http_headers *headers) {
	headers->headers_capacity = 0;
	headers->headers_length = 0;
	headers->headers = NULL;
}

void http_headers_dealloc(http_headers *headers) {
	if (headers->headers_capacity > 0) {
		for (size_t i = 0; i < headers->headers_capacity; i++) {
			http_header_dealloc(&headers->headers[i]);
		}
		free(headers->headers);
	}
}

size_t http_headers_get_num(http_headers *headers) {
	return headers->headers_length;
}

http_header *http_headers_get(http_headers *headers, size_t i) {
	if (i >= headers->headers_length) {
		return NULL;
	}
	return &headers->headers[i];
}

void http_headers_clear(http_headers *headers) {
	headers->headers_length = 0;
}

http_header *http_headers_get_cstr(http_headers *headers, char *name, int create_if_missing) {
	return http_headers_get_cstr_len(headers, name, strlen(name), create_if_missing);
}

http_header *http_headers_get_cstr_len(http_headers *headers, char *name, size_t name_len, int create_if_missing) {
	for (size_t i = 0; i < headers->headers_length; i++) {
		if (!string_compare_cstr_len(&headers->headers[i].name, name, name_len, STRING_COMPARE_CASE_INSENSITIVE)) {
			return &headers->headers[i];
		}
	}
	if (!create_if_missing) {
		return NULL;
	}
	if (headers->headers_length == headers->headers_capacity) {
		if (headers->headers_capacity == 0) {
			headers->headers = malloc(sizeof(http_header));
		} else {
			headers->headers = realloc(headers->headers, sizeof(http_header) * (headers->headers_capacity + 1));
		}
		http_header_init(&headers->headers[headers->headers_capacity]);
		headers->headers_capacity++;
	}
	http_header *result = &headers->headers[headers->headers_length];
	http_header_clear(result);
	string_set_cstr_len(http_header_get_name(result), name, name_len);
	headers->headers_length++;
	return result;
}

// private
void http_headers_to_string(http_headers *headers, string *dst, char *indent) {
	if (headers) {
		string_append_cstrf(dst, "%sheaders, len=%zu\n", indent, http_headers_get_num(headers));
		for (size_t i = 0; i < http_headers_get_num(headers); i++) {
			http_header *header = http_headers_get(headers, i);
			string_append_cstrf(dst, "%s%s%s: ", indent, indent, string_get_cstr(http_header_get_name(header)));
			for (size_t j = 0; j < http_header_get_num_values(header); j++) {
				if (j > 0) {
					string_append_cstr(dst, ",");
				}
				string_append_cstrf(dst, string_get_cstr(http_header_get_value(header, j)));
			}
			string_append_cstr(dst, "\n");
		}
	} else {
		string_append_cstrf(dst, "%sheaders, 0\n", indent);
	}
}

void http_request_init(http_request *request) {
	buffer_init(&request->read_buf);
	string_init(&request->scratch);
	string_init(&request->method);
	string_init(&request->uri);
	http_headers_init(&request->headers);
	buffer_init(&request->body);
}

void http_request_dealloc(http_request *request) {
	buffer_dealloc(&request->read_buf);
	string_dealloc(&request->scratch);
	string_dealloc(&request->method);
	string_dealloc(&request->uri);
	http_headers_dealloc(&request->headers);
	buffer_dealloc(&request->body);
}

string *http_request_get_method(http_request *request) {
	return &request->method;
}

string *http_request_get_uri(http_request *request) {
	return &request->uri;
}

http_headers *http_request_get_headers(http_request *request) {
	return &request->headers;
}

int http_request_parse(http_request *request, stream *stream) {
	buffer_clear(&request->read_buf);
	string_clear(&request->method);
	string_clear(&request->uri);
	http_headers_clear(&request->headers);
	buffer_clear(&request->body);

	size_t end_of_last_line = 0;
	int found_request_line = 0;
	size_t end_of_request_line;
	int found_end_of_header = 0;
	size_t start_of_body;

	// use max size -1 because we're going to want to put a terminating 0
	size_t max_read_length = MAX_SOCKET_READ_SIZE_IN_CHUNKS * CHUNK_READ_SIZE;
	size_t expected_content_length = 0;
	while (buffer_get_length(&request->read_buf) < max_read_length) {
		int read_result = stream_read_buffer(stream, &request->read_buf, CHUNK_READ_SIZE, &request->scratch);
		if (read_result < 0) {
			log_error("parsing http request failed, error reading: %s\n", string_get_cstr(&request->scratch));
			return 1;
		}
		if (read_result == 0) {
			break;
		}
		if (!found_request_line || !found_end_of_header) {
			for (size_t i = end_of_last_line; i < buffer_get_length(&request->read_buf) - 1; i++) {
				// rfc says all lines before the body start with CRLF
				if (request->read_buf.data[i] == '\r' && request->read_buf.data[i + 1] == '\n') {
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
						for (size_t j = 0; j < end_of_request_line; j++) {
							int is_space = request->read_buf.data[j] == ' ';
							if (!found_start_of_method) {
								if (!is_space) {
									found_start_of_method = 1;
									method_start = j;
								}
							} else if (!found_end_of_method) {
								if (is_space) {
									found_end_of_method = 1;
									method_end = j;
								}
							} else if (!found_start_of_uri) {
								if (!is_space) {
									found_start_of_uri = 1;
									uri_start = j;
								}
							} else if (!found_end_of_uri) {
								if (is_space) {
									found_end_of_uri = 1;
									uri_end = j;
								}
							} else if (!found_start_of_protocol_version) {
								if (!is_space) {
									found_start_of_protocol_version = 1;
									protocol_version_start = j;
								}
							} else if (!found_end_of_protocol_version) {
								if (is_space) {
									found_end_of_protocol_version = 1;
									protocol_version_end = j;
									break;
								} else if (j == end_of_request_line - 1) {
									found_end_of_protocol_version = 1;
									protocol_version_end = j + 1;
									break;
								}
							}
						}
						if (found_end_of_protocol_version) {
							string_set_cstr_len(&request->method, request->read_buf.data + method_start, method_end - method_start);
							string_set_cstr_len(&request->uri, request->read_buf.data + uri_start, uri_end - uri_start);
						} else {
							string_set_cstr_len(&request->scratch, request->read_buf.data, end_of_request_line);
							log_error("failed to parse request line: %s\n", string_get_cstr(&request->scratch));
							return 1;
						}
					} else if (!found_end_of_header) {
						if (line_length == 0) {
							found_end_of_header = 1;
							start_of_body = i + 2;
							// at this point we either have a content length header or we don't
							// either way we know exactly how many more bytes to read, which may be 0
							http_header *header = http_headers_get_cstr(&request->headers, "Content-Length",
																		// don't create this header if missing
																		0);
							if (!header) {
								expected_content_length = 0;
								max_read_length = buffer_get_length(&request->read_buf);
							} else if (http_header_get_num_values(header) != 1) {
								log_error("Content-Length has wrong number of values, expected 1 got %zu\n",
										  http_header_get_num_values(header));
								return 1;
							} else {
								string *value = http_header_get_value(header, 0);
								if (sscanf(string_get_cstr(value), "%zu", &expected_content_length) == 1) {
									max_read_length = i + expected_content_length;
								} else {
									log_error("Content-Length header present but isn't a valid integer: %s\n", string_get_cstr(value));
									return 1;
								}
							}
							break;
						} else {
							string_set_cstr_len(&request->scratch, request->read_buf.data + end_of_last_line, line_length);
							size_t split_results[4];
							// there can only be 2 maximum results here separating key and value
							size_t split_count = string_split(&request->scratch, 0, ":", split_results, 2, 2);
							if (split_count != 2) {
								log_error("failed to parse header line: %s\n", string_get_cstr(&request->scratch));
								return 1;
							}
							http_header *header =
								http_headers_get_cstr_len(&request->headers, string_get_cstr(&request->scratch) + split_results[0],
														  split_results[1] - split_results[0],
														  // create this header if missing
														  1);

							// scratch is now the value with any leading whitespace trimmed off
							string_set_substr(&request->scratch, &request->scratch, split_results[2], -1);
							string_trim_start_any_of_cstr(&request->scratch, &request->scratch, " \t");
							// split off the first value by looking for a comma
							size_t num_value_splits = string_split(&request->scratch, 0, ",", split_results, 2, 2);
							while (1) {
								// there can only be exactly 1 or 2 such splits, either there was a comma or there wasn't
								// in either case the first result in the splits will be the next value
								string *value = http_header_append_value(header);
								string_set_substr(value, &request->scratch, split_results[0], split_results[1]);
								// if we had a 2nd value that means there is at least one more value
								// since we were only allowing a max of 2 results there might still be multiple values packed in here
								if (num_value_splits == 2) {
									num_value_splits = string_split(&request->scratch, split_results[2], ",", split_results, 2, 2);
								} else {
									// we're done there was only 1 split
									break;
								}
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

	if (!found_end_of_header) {
		log_error("failed to find end of headers\n");
		return 1;
	}

	buffer_append_bytes(&request->body, request->read_buf.data + start_of_body, buffer_get_length(&request->read_buf) - start_of_body);

	string_clear(&request->scratch);
	string_append_cstrf(&request->scratch, "parsed request %s %s\n", string_get_cstr(&request->method), string_get_cstr(&request->uri));
	http_headers_to_string(&request->headers, &request->scratch, "    ");
	string_append_cstrf(&request->scratch, "    body: %zu bytes\n", buffer_get_length(&request->body));
	log_trace("%s\n", string_get_cstr(&request->scratch));

	if (buffer_get_length(&request->body) != expected_content_length) {
		log_error("expected content length %zu but read %zu bytes\n", expected_content_length, buffer_get_length(&request->body));
		return 1;
	}

	return 0;
}

void http_response_init(http_response *response) {
	string_init(&response->scratch);
	string_init(&response->reason_phrase);
	http_response_set_status_code(response, 200);
	http_headers_init(&response->headers);
	buffer_init(&response->body_buffer);
	stream_init_buffer(&response->body_stream, &response->body_buffer, 0);
}

void http_response_dealloc(http_response *response) {
	http_headers_dealloc(&response->headers);
	buffer_dealloc(&response->body_buffer);
	if (stream_dealloc(&response->body_stream, &response->scratch)) {
		log_error("error deallocating response body stream: %s\n", &response->scratch);
	}
	string_dealloc(&response->scratch);
}

void http_response_clear(http_response *response) {
	string_clear(&response->scratch);
	http_response_set_status_code(response, 200);
	http_headers_clear(&response->headers);
	buffer_clear(&response->body_buffer);
	stream_set_position(&response->body_stream, 0);
}

int http_response_get_status_code(http_response *response) {
	return response->status_code;
}

void http_response_set_status_code(http_response *response, int status_code) {
	response->status_code = status_code;
	switch (status_code) {
	case 100:
		string_set_cstr(&response->reason_phrase, "Continue");
		break;
	case 101:
		string_set_cstr(&response->reason_phrase, "Switching Protocols");
		break;
	case 200:
		string_set_cstr(&response->reason_phrase, "OK");
		break;
	case 201:
		string_set_cstr(&response->reason_phrase, "Created");
		break;
	case 202:
		string_set_cstr(&response->reason_phrase, "Accepted");
		break;
	case 203:
		string_set_cstr(&response->reason_phrase, "Non-Authoritative Information");
		break;
	case 204:
		string_set_cstr(&response->reason_phrase, "No Content");
		break;
	case 205:
		string_set_cstr(&response->reason_phrase, "Reset Content");
		break;
	case 206:
		string_set_cstr(&response->reason_phrase, "Partial Content");
		break;
	case 300:
		string_set_cstr(&response->reason_phrase, "Multiple Choices");
		break;
	case 301:
		string_set_cstr(&response->reason_phrase, "Moved Permanently");
		break;
	case 302:
		string_set_cstr(&response->reason_phrase, "Found");
		break;
	case 303:
		string_set_cstr(&response->reason_phrase, "See Other");
		break;
	case 304:
		string_set_cstr(&response->reason_phrase, "Not Modified");
		break;
	case 305:
		string_set_cstr(&response->reason_phrase, "Use Proxy");
		break;
	case 307:
		string_set_cstr(&response->reason_phrase, "Temporary Redirect");
		break;
	case 400:
		string_set_cstr(&response->reason_phrase, "Bad Request");
		break;
	case 401:
		string_set_cstr(&response->reason_phrase, "Unauthorized");
		break;
	case 402:
		string_set_cstr(&response->reason_phrase, "Payment Required");
		break;
	case 403:
		string_set_cstr(&response->reason_phrase, "Forbidden");
		break;
	case 404:
		string_set_cstr(&response->reason_phrase, "Not Found");
		break;
	case 405:
		string_set_cstr(&response->reason_phrase, "Method Not Allowed");
		break;
	case 406:
		string_set_cstr(&response->reason_phrase, "Not Acceptable");
		break;
	case 407:
		string_set_cstr(&response->reason_phrase, "Proxy Authentication Required");
		break;
	case 408:
		string_set_cstr(&response->reason_phrase, "Request Time-out");
		break;
	case 409:
		string_set_cstr(&response->reason_phrase, "Conflict");
		break;
	case 410:
		string_set_cstr(&response->reason_phrase, "Gone");
		break;
	case 411:
		string_set_cstr(&response->reason_phrase, "Length Required");
		break;
	case 412:
		string_set_cstr(&response->reason_phrase, "Precondition Failed");
		break;
	case 413:
		string_set_cstr(&response->reason_phrase, "Request Entity Too Large");
		break;
	case 414:
		string_set_cstr(&response->reason_phrase, "Request-URI Too Large");
		break;
	case 415:
		string_set_cstr(&response->reason_phrase, "Unsupported Media Type");
		break;
	case 416:
		string_set_cstr(&response->reason_phrase, "Requested range not satisfiable");
		break;
	case 417:
		string_set_cstr(&response->reason_phrase, "Expectation Failed");
		break;
	case 500:
		string_set_cstr(&response->reason_phrase, "Internal Server Error");
		break;
	case 501:
		string_set_cstr(&response->reason_phrase, "Not Implemented");
		break;
	case 502:
		string_set_cstr(&response->reason_phrase, "Bad Gateway");
		break;
	case 503:
		string_set_cstr(&response->reason_phrase, "Service Unavailable");
		break;
	case 504:
		string_set_cstr(&response->reason_phrase, "Gateway Time-out");
		break;
	case 505:
		string_set_cstr(&response->reason_phrase, "HTTP Version not supported");
		break;
	default:
		string_set_cstr(&response->reason_phrase, "Other");
	}
}

string *http_response_get_reason_phrase(http_response *response) {
	return &response->reason_phrase;
}

http_headers *http_response_get_headers(http_response *response) {
	return &response->headers;
}

stream *http_response_get_body(http_response *response) {
	return &response->body_stream;
}

int http_response_write(http_response *response, stream *stream) {
	// fix the content length header first
	// this is true for even empty bodies, as without a Content-Length of 0 the client may not properly handle the response
	// it's possible that a more careful reading of the RFC would make this obvious, but I see Content-Length as optional
	// add it anyway to make clients happy
	http_header *content_length_header = http_headers_get_cstr(&response->headers, "Content-Length", 1);
	// check to see if the content length header is already set to the correct value
	// check number of values, anything but exactly one value is obviously not correct
	if (http_header_get_num_values(content_length_header) != 1) {
		http_header_clear(content_length_header);
	} else {
		// get the actual value
		size_t content_length_header_value;
		if (sscanf(string_get_cstr(http_header_get_value(content_length_header, 0)), "%zu", &content_length_header_value) == 1) {
			// it's an intenger, is it the right value already?
			if (content_length_header_value != buffer_get_length(&response->body_buffer)) {
				http_header_clear(content_length_header);
			}
		} else {
			// not the right value
			http_header_clear(content_length_header);
		}
	}
	// if we ended up clearing the header add the correct value back
	if (http_header_get_num_values(content_length_header) == 0) {
		string_set_cstrf(http_header_append_value(content_length_header), "%zu", buffer_get_length(&response->body_buffer));
	}

	string_clear(&response->scratch);
	string_append_cstrf(&response->scratch, "serializing response %i %s\n", response->status_code,
						string_get_cstr(&response->reason_phrase));
	http_headers_to_string(&response->headers, &response->scratch, "    ");
	string_append_cstrf(&response->scratch, "    body: %zu bytes\n", buffer_get_length(&response->body_buffer));
	log_trace("%s\n", string_get_cstr(&response->scratch));

	// status line
	if (stream_write_cstrf(stream, &response->scratch, "HTTP/1.1 %i %s\r\n", response->status_code,
						   string_get_cstr(&response->reason_phrase)) < 0) {
		log_error("error writing status line: %s\n", string_get_cstr(&response->scratch));
		return 1;
	}

	// headers
	for (size_t i = 0; i < http_headers_get_num(&response->headers); i++) {
		http_header *header = http_headers_get(&response->headers, i);
		if (stream_write_cstrf(stream, &response->scratch, "%s: ", string_get_cstr(http_header_get_name(header))) < 0) {
			log_error("error writing header name (%zu): %s\n", i, string_get_cstr(&response->scratch));
			return 1;
		}
		for (size_t j = 0; j < http_header_get_num_values(header); j++) {
			string *value = http_header_get_value(header, j);
			if (j > 0) {
				if (stream_write_cstrf(stream, &response->scratch, ",") < 0) {
					log_error("error writing header value separator (%zu, %zu): %s\n", i, j, string_get_cstr(&response->scratch));
					return 1;
				}
			}
			if (stream_write_cstrf(stream, &response->scratch, "%s", string_get_cstr(value)) < 0) {
				log_error("error writing header value (%zu, %zu): %s\n", i, j, string_get_cstr(&response->scratch));
				return 1;
			}
		}
		if (stream_write_cstrf(stream, &response->scratch, "\r\n") < 0) {
			log_error("error writing header line break (%zu): %s\n", i, string_get_cstr(&response->scratch));
			return 1;
		}
	}

	// separator between headers and body
	if (stream_write_cstrf(stream, &response->scratch, "\r\n") < 0) {
		log_error("error writing extra line break between headers and body: %s\n", string_get_cstr(&response->scratch));
		return 1;
	}

	// body
	if (stream_write_buffer(stream, &response->body_buffer, &response->scratch) < 0) {
		log_error("error writing body: %s\n", string_get_cstr(&response->scratch));
		return 1;
	}

	// success
	return 0;
}

// private
void http_server_finalize_task(http_server_task_data *data) {
	log_trace("responding to request %s:%i %s %s\n", string_get_cstr(&data->request_address), data->request_port,
			  string_get_cstr(http_request_get_method(&data->request)), string_get_cstr(http_request_get_uri(&data->request)));
	if (http_response_write(&data->response, &data->socket_stream)) {
		log_error("failed to write HTTP response to the socket stream\n");
	}
	// close out future reads and writes
	// if we're writing a timeout response this will cause future writes to the socket to fail in the handler function ever finishes
	shutdown(data->socket, SHUT_RDWR);
	if (stream_dealloc(&data->socket_stream, &data->scratch)) {
		log_error("failed to close HTTP request socket stream: %s\n", string_get_cstr(&data->scratch));
	}

	// put task back on pool
	if (pthread_mutex_lock(&data->server->task_pool_mutex)) {
		log_error("error locking http server task pool\n");
		close(data->socket);
		return;
	}
	data->next = data->server->task_pool;
	data->server->task_pool = data;
	data->server->task_pool_len++;
	log_trace("put http server task data back on the pool, there are now %zu tasks in the pool\n", data->server->task_pool_len);
	if (pthread_mutex_unlock(&data->server->task_pool_mutex)) {
		log_error("error unlock http server task pool\n");
	}
}

// private
int http_server_task(int thread_id, void *data) {
	http_server_task_data *task_data = data;

	// parse the input
	log_trace("parsing incoming HTTP request from %s:%i\n", string_get_cstr(&task_data->request_address), task_data->request_port);
	if (http_request_parse(&task_data->request, &task_data->socket_stream)) {
		// failed to even read the input document, just return an error telling the client they did this wrong
		log_error("error parsing HTTP data\n");
		http_response_clear(&task_data->response);
		http_response_set_status_code(&task_data->response, 400);
		goto DONE;
	}

	// try to handle this with the user-provided callback
	http_response_clear(&task_data->response);
	log_trace("handling HTTP request from %s:%i %s %s\n", string_get_cstr(&task_data->request_address), task_data->request_port,
			  string_get_cstr(http_request_get_method(&task_data->request)), string_get_cstr(http_request_get_uri(&task_data->request)));
	if (task_data->server->callback(task_data->server->callback_data, &task_data->request, &task_data->response)) {
		// something bad happened in the handler, just return a generic error
		log_debug("HTTP handler failed\n");
		http_response_clear(&task_data->response);
		http_response_set_status_code(&task_data->response, 500);
		goto DONE;
	}

DONE:
	http_server_finalize_task(task_data);
	return 0;
}

// private
void http_server_socket_accept(void *data, string *address, uint16_t port, int socket) {
	http_server *server = data;

	// grab a task off the pool
	if (pthread_mutex_lock(&server->task_pool_mutex)) {
		log_error("error locking http server task pool\n");
		close(socket);
		return;
	}
	http_server_task_data *task_data;
	if (server->task_pool) {
		// a task is available so use that
		task_data = server->task_pool;
		server->task_pool = server->task_pool->next;
		server->task_pool_len--;
		log_trace("http server task pool had an available task, there are %zu tasks remaining in the pool\n", server->task_pool_len);
	} else {
		// allocate a new task
		log_trace("http server task pool was empty, allocating new task data\n");
		task_data = malloc(sizeof(http_server_task_data));
		task_data->server = server;
		string_init(&task_data->scratch);
		string_init(&task_data->request_address);
		http_request_init(&task_data->request);
		http_response_init(&task_data->response);
	}
	if (pthread_mutex_unlock(&server->task_pool_mutex)) {
		log_error("error unlock http server task pool\n");
	}

	// remember where this request came from
	string_set_str(&task_data->request_address, address);
	task_data->request_port = port;
	log_trace("queuing incoming HTTP request from %s:%i\n", string_get_cstr(&task_data->request_address), task_data->request_port);

	// fill in the socket on the task
	task_data->socket = socket;
	stream_init_file_descriptor(&task_data->socket_stream, socket, 1);

	// try to handle this on the thread pool
	int enqueue_error = worker_thread_pool_enqueue(&server->thread_pool, http_server_task, task_data, NULL, server->timeout);
	int should_finalize_task = 0;
	switch (enqueue_error) {
	case 0:
		// nothing to do, this is the success case
		// we can assume the worker thread will clean up everything
		return;
	case WORKER_THREAD_POOL_ERROR_QUEUE_FULL:
		// we didn't enqueue, so we can't rely on them to free memory
		log_error("failed to execute incoming HTTP request, queue is full\n");
		http_response_clear(&task_data->response);
		http_response_set_status_code(&task_data->response, 503);
		should_finalize_task = 1;
		break;
	case WORKER_THREAD_POOL_ERROR_TIMEOUT:
		// we did enqueue, it's just taking a long time
		log_error("timed out waiting on HTTP response handler for request\n");
		http_response_clear(&task_data->response);
		http_response_set_status_code(&task_data->response, 503);
		should_finalize_task = 1;
		break;
	default:
		// any other error we assume we didn't end up in the queue so clean up
		log_error("failed to execute incoming request, %i\n", enqueue_error);
		http_response_clear(&task_data->response);
		http_response_set_status_code(&task_data->response, 500);
		should_finalize_task = 1;
		break;
	}
	// if we had some kind of error that means we're going to need to respond ourselves, instead of letter the user callback handle it, we
	// can do so here
	if (should_finalize_task) {
		http_server_finalize_task(task_data);
	}
}

int http_server_init(http_server *server, http_server_func callback, void *callback_data, char *address, uint16_t port, int num_threads,
					 int queue_size, uint64_t timeout) {
	server->callback = callback;
	server->timeout = timeout;

	int result = 0;
	int socket_init = 0;
	int thread_pool_init = 0;
	int mutex_init = 0;

	if (tcp_socket_wrapper_init(&server->socket, address, port, http_server_socket_accept, server)) {
		log_error("failed to open the http server socket\n");
		result = 1;
		goto DONE;
	}
	socket_init = 1;

	if (worker_thread_pool_init(&server->thread_pool, num_threads, queue_size)) {
		log_error("failed to initialize the http server thread pool\n");
		result = 1;
		goto DONE;
	}
	thread_pool_init = 1;

	if (pthread_mutex_init(&server->task_pool_mutex, NULL)) {
		log_error("failed to allocate mutex for task pool\n");
		result = 1;
		goto DONE;
	}
	mutex_init = 1;
	server->task_pool_len = 0;
	server->task_pool = NULL;

	log_debug("http server started at %s:%i\n", string_get_cstr(tcp_socket_wrapper_get_address(&server->socket)),
			  tcp_socket_wrapper_get_port(&server->socket));
DONE:
	if (result) {
		if (socket_init && tcp_socket_wrapper_dealloc(&server->socket)) {
			log_error("failed to close the http server socket after a previous failure to initialize the http server\n");
		}
		if (thread_pool_init && worker_thread_pool_dealloc(&server->thread_pool)) {
			log_error("failed to clean up the thread pool after a previous failure to initialize the http server\n");
		}
		if (mutex_init && pthread_mutex_destroy(&server->task_pool_mutex)) {
			log_error("failed to clean up task pool mutex after a previous failure to initialize the http server\n");
		}
	}
	return result;
}

int http_server_dealloc(http_server *server) {
	int result = 0;
	if (tcp_socket_wrapper_dealloc(&server->socket)) {
		log_error("failed to close the http server socket\n");
		result = 1;
	}
	if (worker_thread_pool_dealloc(&server->thread_pool)) {
		log_error("failed to close the http server thread pool\n");
		result = 1;
	}
	if (pthread_mutex_destroy(&server->task_pool_mutex)) {
		log_error("failed to clean up the task pool mutex\n");
		result = 1;
	}
	while (server->task_pool) {
		http_server_task_data *data = server->task_pool;
		server->task_pool = server->task_pool->next;
		string_dealloc(&data->scratch);
		string_dealloc(&data->request_address);
		http_request_dealloc(&data->request);
		http_response_dealloc(&data->response);
		free(data);
	}
	free(server->task_pool);
	return result;
}