#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "http.h"
#include "log.h"

// TODO use a reasonable chunk size
#define CHUNK_READ_SIZE 50
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

int http_request_parse(http_request *request, io *io) {
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
		int read_result = io_read_buffer(io, &request->read_buf, CHUNK_READ_SIZE, &request->scratch);
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
								// TODO JEFF set max read length to something sensible?
								// max_read_length = buffer_get_length(&request->read_buf);
							} else if (http_header_get_num_values(header) != 1) {
								log_error("Content-Length has wrong number of values, expected 1 got %zu\n",
										  http_header_get_num_values(header));
								return 1;
							} else {
								string *value = http_header_get_value(header, 0);
								if (sscanf(string_get_cstr(value), "%zu", &expected_content_length) == 1) {
									// TODO JEFF set max read length to something sensible?
									// max_read_length = i + expected_content_length;
								} else {
									log_error("Content-Length header present but isn't a valid integer: %s\n", string_get_cstr(value));
									return 1;
								}
							}
							break;
						} else {
							// TODO JEFF header parsing isn't correct yet, need to split around commas
							string_set_cstr_len(&request->scratch, request->read_buf.data + end_of_last_line, line_length);
							// splitting gets used for finding multiple values separated by commas
							// just guess a max number, and if there are more than that we'll just iterate
							size_t max_split_results = 3;
							size_t split_results[max_split_results * 2];
							// there can only be 2 maximum results here separating key and value
							size_t split_count = string_split(&request->scratch, ":", split_results, max_split_results, 2);
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
							log_trace("TODO JEFF before split, value = %s\n", string_get_cstr(&request->scratch));

							// best effort at splitting the value up
							size_t total_values = string_split(&request->scratch, ",", split_results, max_split_results, max_split_results);
							log_trace("TODO JEFF we think there are %zu values here\n", total_values);
							while (1) {
								// should be impossible, there must be at least one substring in the source string
								if (total_values == 0) {
									log_error("failed to split value string %s\n", &request->scratch);
									return 1;
								}
								// if there were multiple values the last one doesn't count, it might still have separators
								size_t real_split_count;
								if (total_values == 1) {
									real_split_count = total_values;
								} else {
									real_split_count = total_values - 1;
								}
								// divide up the ones we're sure of
								size_t split;
								for (size_t split = 0; split < real_split_count * 2; split += 2) {
									string *value = http_header_append_value(header);
									string_set_substr(value, &request->scratch, split_results[split], split_results[split + 1]);
									log_trace("TODO JEFF found value %s\n", string_get_cstr(value));
								}
								// done because the last value must not have had any separators in it
								if (total_values < max_split_results) {
									break;
								}
								// last value might be full of separators and really be multiple values
								// TODO JEFF could just be a split call that takes a start index
								string_set_substr(&request->scratch, &request->scratch, split_results[split], split_results[split + 1]);
								log_trace("TODO JEFF looping, new string to look at is %s\n", string_get_cstr(&request->scratch));
								total_values = string_split(&request->scratch, ",", split_results, max_split_results, max_split_results);
								log_trace("TODO JEFF new remaining splits %zu\n", total_values);
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

	// TODO merge trace logging into a single statement
	string_clear(&request->scratch);
	string_append_cstrf(&request->scratch, "received request %s %s\n", string_get_cstr(&request->method),
						string_get_cstr(&request->method));
	for (size_t i = 0; i < http_headers_get_num(&request->headers); i++) {
		http_header *header = http_headers_get(&request->headers, i);
		string_append_cstrf(&request->scratch, "    %s", string_get_cstr(http_header_get_name(header)));
		for (size_t j = 0; j < http_header_get_num_values(header); j++) {
			if (j > 0) {
				string_append_cstr(&request->scratch, ", ");
			}
			string_append_cstrf(&request->scratch, string_get_cstr(http_header_get_value(header, j)));
		}
		string_append_cstr(&request->scratch, "\n");
	}
	string_append_cstrf(&request->scratch, "    body: %zu\n", buffer_get_length(&request->body));

	if (buffer_get_length(&request->body) != expected_content_length) {
		log_error("expected content length %zu but read %zu bytes\n", expected_content_length, buffer_get_length(&request->body));
		return 1;
	}

	return 0;
}
