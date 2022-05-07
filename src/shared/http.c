#include <errno.h>
#include <stdio.h>
#include <string.h>
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
	string_clear(&header->name);
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
							size_t start_of_value = split_results[2];
							size_t first_non_whitespace = string_index_not_of_any_cstr(&request->scratch, " \t", start_of_value);
							if (first_non_whitespace >= 0) {
								start_of_value = first_non_whitespace;
							}
							string_set_cstr_len(&request->scratch, string_get_cstr(&request->scratch) + start_of_value,
												split_results[3] - start_of_value);
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

int http_response_write(stream *dst, int status_code, http_headers *headers) {
	string scratch, error;
	string_init(&scratch);
	string_init(&error);
	int result;

	string_append_cstrf(&scratch, "    status_code %i\n", status_code);
	http_headers_to_string(headers, &scratch, "    ");
	log_trace("TODO JEFF http_response_write\n%s\n", string_get_cstr(&scratch));

	char *reason_phrase;
	switch (status_code) {
	case 100:
		reason_phrase = "Continue";
		break;
	case 101:
		reason_phrase = "Switching Protocols";
		break;
	case 200:
		reason_phrase = "OK";
		break;
		/*
		TODO JEFF rest of the reason phrases
		  | "201"  ; Section 10.2.2: Created
		  | "202"  ; Section 10.2.3: Accepted
		  | "203"  ; Section 10.2.4: Non-Authoritative Information
		  | "204"  ; Section 10.2.5: No Content
		  | "205"  ; Section 10.2.6: Reset Content
		  | "206"  ; Section 10.2.7: Partial Content
		  | "300"  ; Section 10.3.1: Multiple Choices
		  | "301"  ; Section 10.3.2: Moved Permanently
		  | "302"  ; Section 10.3.3: Found
		  | "303"  ; Section 10.3.4: See Other
		  | "304"  ; Section 10.3.5: Not Modified
		  | "305"  ; Section 10.3.6: Use Proxy
		  | "307"  ; Section 10.3.8: Temporary Redirect
		  | "400"  ; Section 10.4.1: Bad Request
		  | "401"  ; Section 10.4.2: Unauthorized
		  | "402"  ; Section 10.4.3: Payment Required
		  | "403"  ; Section 10.4.4: Forbidden
		  | "404"  ; Section 10.4.5: Not Found
		  | "405"  ; Section 10.4.6: Method Not Allowed
		  | "406"  ; Section 10.4.7: Not Acceptable
			| "407"  ; Section 10.4.8: Proxy Authentication Required
		  | "408"  ; Section 10.4.9: Request Time-out
		  | "409"  ; Section 10.4.10: Conflict
		  | "410"  ; Section 10.4.11: Gone
		  | "411"  ; Section 10.4.12: Length Required
		  | "412"  ; Section 10.4.13: Precondition Failed
		  | "413"  ; Section 10.4.14: Request Entity Too Large
		  | "414"  ; Section 10.4.15: Request-URI Too Large
		  | "415"  ; Section 10.4.16: Unsupported Media Type
		  | "416"  ; Section 10.4.17: Requested range not satisfiable
		  | "417"  ; Section 10.4.18: Expectation Failed
		  | "500"  ; Section 10.5.1: Internal Server Error
		  | "501"  ; Section 10.5.2: Not Implemented
		  | "502"  ; Section 10.5.3: Bad Gateway
		  | "503"  ; Section 10.5.4: Service Unavailable
		  | "504"  ; Section 10.5.5: Gateway Time-out
		  | "505"  ; Section 10.5.6: HTTP Version not supported
	*/
	default:
		reason_phrase = "Other";
	}

	// status line
	if (stream_write_cstrf(dst, &error, "HTTP/1.1 %i %s\r\n", status_code, reason_phrase) < 0) {
		log_error("error writing status line: %s\n", string_get_cstr(&error));
		result = 1;
		goto DONE;
	}

	// TODO JEFF write headers

	// success
	result = 0;
DONE:
	string_dealloc(&error);
	string_dealloc(&scratch);
	return result;
}

int http_response_write_data(stream *dst, int status_code, http_headers *headers, void *body, size_t body_len) {
	// headers are optionally, but we might need to create some if we have a response body
	http_headers local_headers;
	int should_dealloc_headers = 0;
	if (body && body_len > 0) {
		// allocate some headers if needed
		if (!headers) {
			http_headers_init(&local_headers);
			should_dealloc_headers = 1;
			headers = &local_headers;
		}
		// update content length to match the given body
		http_header *content_length_header = http_headers_get_cstr(headers, "Content-Length", 1);
		http_header_clear(content_length_header);
		string *content_length_value = http_header_append_value(content_length_header);
		string_append_cstrf(content_length_value, "%zu", body_len);
	}
	// write everything but the body
	int result = http_response_write(dst, status_code, headers);
	if (result) {
		goto DONE;
	}
	// write the body
	string error;
	string_init(&error);
	result = stream_write(dst, body, body_len, &error);
	if (result < 0) {
		log_error("error writing response body to stream: %s\n", string_get_cstr(&error));
		goto DONE;
	}
	string_dealloc(&error);
	// success
	result = 0;
DONE:
	if (should_dealloc_headers) {
		http_headers_dealloc(&local_headers);
	}
	return result;
}

int http_response_write_buffer(stream *dst, int status_code, http_headers *headers, buffer *body) {
	if (!body || buffer_get_length(body) == 0) {
		return http_response_write(dst, status_code, headers);
	}
	return http_response_write_data(dst, status_code, headers, body->data, body->length);
}

int http_response_write_str(stream *dst, int status_code, http_headers *headers, string *body) {
	if (!body || string_get_length(body) == 0) {
		return http_response_write(dst, status_code, headers);
	}
	return http_response_write_data(dst, status_code, headers, string_get_cstr(body), string_get_length(body));
}

int http_response_write_cstr(stream *dst, int status_code, http_headers *headers, char *body) {
	size_t len = 0;
	if (body) {
		len = strlen(body);
	}
	if (!body || len == 0) {
		return http_response_write(dst, status_code, headers);
	}
	return http_response_write_data(dst, status_code, headers, body, len);
}

int http_response_write_stream(stream *dst, int status_code, http_headers *headers, stream *body) {
	if (!body) {
		return http_response_write(dst, status_code, headers);
	}
	buffer b;
	buffer_init(&b);
	int result = stream_read_all_into_buffer(body, &b, 0, 1024, NULL);
	if (result > 0) {
		result = http_response_write_buffer(dst, status_code, headers, &b);
	}
	buffer_dealloc(&b);
	return result;
}
