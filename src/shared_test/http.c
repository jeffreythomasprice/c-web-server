#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "../shared/http.h"

void header() {
	http_header header;
	http_header_init(&header);

	assert(string_compare_cstr(http_header_get_name(&header), "", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_header_get_num_values(&header) == 0);

	string_set_cstr(http_header_get_name(&header), "foo");
	string_set_cstr(http_header_append_value(&header), "bar");
	string_set_cstr(http_header_append_value(&header), "baz");

	assert(string_compare_cstr(http_header_get_name(&header), "foo", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_header_get_num_values(&header) == 2);
	assert(http_header_get_value(&header, 0) != NULL);
	assert(http_header_get_value(&header, 1) != NULL);
	assert(http_header_get_value(&header, 2) == NULL);
	assert(string_compare_cstr(http_header_get_value(&header, 0), "bar", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr(http_header_get_value(&header, 1), "baz", STRING_COMPARE_CASE_SENSITIVE) == 0);

	http_header_clear(&header);

	string_set_cstr(http_header_get_name(&header), "Content-Length");
	string_set_cstr(http_header_append_value(&header), "1234");

	assert(string_compare_cstr(http_header_get_name(&header), "Content-Length", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_header_get_num_values(&header) == 1);
	assert(http_header_get_value(&header, 0) != NULL);
	assert(http_header_get_value(&header, 1) == NULL);
	assert(string_compare_cstr(http_header_get_value(&header, 0), "1234", STRING_COMPARE_CASE_SENSITIVE) == 0);

	http_header_dealloc(&header);
}

void headers() {
	http_headers headers;
	http_headers_init(&headers);

	assert(http_headers_get_num(&headers) == 0);

	string_set_cstr(http_header_append_value(http_headers_get_cstr(&headers, "foo", 1)), "one");
	string_set_cstr(http_header_append_value(http_headers_get_cstr(&headers, "foo", 1)), "two");
	string_set_cstr(http_header_append_value(http_headers_get_cstr(&headers, "bar", 1)), "three");

	assert(http_headers_get_num(&headers) == 2);
	assert(string_compare_cstr(http_header_get_name(http_headers_get(&headers, 0)), "foo", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_headers_get_cstr(&headers, "foo", 0) == http_headers_get(&headers, 0));
	assert(http_header_get_num_values(http_headers_get(&headers, 0)) == 2);
	assert(string_compare_cstr(http_header_get_value(http_headers_get(&headers, 0), 0), "one", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr(http_header_get_value(http_headers_get(&headers, 0), 1), "two", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(string_compare_cstr(http_header_get_name(http_headers_get(&headers, 1)), "bar", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_headers_get_cstr(&headers, "bar", 0) == http_headers_get(&headers, 1));
	assert(http_header_get_num_values(http_headers_get(&headers, 1)) == 1);
	assert(string_compare_cstr(http_header_get_value(http_headers_get(&headers, 1), 0), "three", STRING_COMPARE_CASE_SENSITIVE) == 0);

	assert(http_headers_get_cstr(&headers, "Foo", 0) == http_headers_get(&headers, 0));
	assert(http_headers_get_cstr(&headers, "foO", 0) == http_headers_get(&headers, 0));
	assert(http_headers_get_cstr(&headers, "foo", 0) == http_headers_get_cstr_len(&headers, "foobarbaz", 3, 0));
	assert(http_headers_get_cstr(&headers, "baz", 0) == NULL);

	http_headers_clear(&headers);

	assert(http_headers_get_num(&headers) == 0);

	string_set_cstr(http_header_append_value(http_headers_get_cstr(&headers, "Content-Length", 1)), "1234");

	assert(http_headers_get_num(&headers) == 1);
	assert(string_compare_cstr(http_header_get_name(http_headers_get(&headers, 0)), "Content-Length", STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_headers_get_cstr(&headers, "Content-Length", 0) == http_headers_get(&headers, 0));
	assert(http_header_get_num_values(http_headers_get(&headers, 0)) == 1);
	assert(string_compare_cstr(http_header_get_value(http_headers_get(&headers, 0), 0), "1234", STRING_COMPARE_CASE_SENSITIVE) == 0);

	http_headers_dealloc(&headers);
}

void assert_parses_successfully(http_request *request, char *input) {
	buffer input_buffer;
	buffer_init_copy(&input_buffer, input, strlen(input));
	io input_io;
	io_init_buffer(&input_io, &input_buffer, 1);

	int parse_result = http_request_parse(request, &input_io);
	assert(parse_result == 0);

	io_dealloc(&input_io, NULL);
}

void assert_method(http_request *request, char *expected) {
	assert(string_compare_cstr(&request->method, expected, STRING_COMPARE_CASE_SENSITIVE) == 0);
}

void assert_uri(http_request *request, char *expected) {
	assert(string_compare_cstr(&request->uri, expected, STRING_COMPARE_CASE_SENSITIVE) == 0);
}

void assert_header(http_request *request, char *expected_name, size_t expected_num_values, ...) {
	va_list args;
	va_start(args, expected_num_values);
	http_header *header = http_headers_get_cstr(&request->headers, expected_name, 0);
	assert(header != NULL);
	assert(string_compare_cstr(http_header_get_name(header), expected_name, STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(http_header_get_num_values(header) == expected_num_values);
	for (size_t i = 0; i < expected_num_values; i++) {
		char *expected_value = va_arg(args, char *);
		log_trace("TODO JEFF header value at %zu = %s, expected value = %s\n", i, string_get_cstr(http_header_get_value(header, i)),
				  expected_value);
		assert(string_compare_cstr(http_header_get_value(header, i), expected_value, STRING_COMPARE_CASE_SENSITIVE) == 0);
	}
	va_end(args);
}

void assert_no_body(http_request *request) {
	assert(buffer_get_length(&request->body) == 0);
}

void parse() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "GET / HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "\r\n");
	assert_method(&request, "GET");
	assert_uri(&request, "/");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_no_body(&request);
	http_request_dealloc(&request);
}

int main() {
	header();
	headers();
	parse();
	return 0;
}