/*
Generating sample data;
curl example.com -X POST  -H "Content-Type: text/plain" --data-binary @input --trace trace.log
*/

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
	stream input_io;
	stream_init_buffer(&input_io, &input_buffer, 1);

	int parse_result = http_request_parse(request, &input_io);
	assert(parse_result == 0);

	stream_dealloc(&input_io, NULL);
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
		assert(string_compare_cstr(http_header_get_value(header, i), expected_value, STRING_COMPARE_CASE_SENSITIVE) == 0);
	}
	va_end(args);
}

void assert_no_body(http_request *request) {
	assert(buffer_get_length(&request->body) == 0);
}

void assert_body(http_request *request, char *expected_body) {
	size_t len = strlen(expected_body);
	assert(buffer_get_length(&request->body) == len);
	assert(!memcmp(request->body.data, expected_body, len));
}

void parse_request_get() {
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

void parse_request_post_no_body() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "POST / HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "\r\n");
	assert_method(&request, "POST");
	assert_uri(&request, "/");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_no_body(&request);
	http_request_dealloc(&request);
}

void parse_request_post_with_body_text() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "POST / HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "Content-Length: 27\r\n"
										 "Content-Type: text/plain\r\n"
										 "\r\n"
										 "Hello, World!\n"
										 "another line\n");
	assert_method(&request, "POST");
	assert_uri(&request, "/");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_header(&request, "Content-Type", 1, "text/plain");
	assert_header(&request, "Content-Length", 1, "27");
	assert_body(&request, "Hello, World!\n"
						  "another line\n");
	http_request_dealloc(&request);
}

void parse_request_post_with_body_json() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "POST /some/api HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "Content-Type: application/json\r\n"
										 "Content-Length: 80\r\n"
										 "\r\n"
										 "{\n"
										 "\t\"foo\": \"bar\",\n"
										 "\t\"baz\": 42,\n"
										 "\t\"blarg\": {\n"
										 "\t\t\"array\": [\n"
										 "\t\t\t1,\n"
										 "\t\t\t2,\n"
										 "\t\t\t3\n"
										 "\t\t]\n"
										 "\t}\n"
										 "}\n");
	assert_method(&request, "POST");
	assert_uri(&request, "/some/api");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_header(&request, "Content-Type", 1, "application/json");
	assert_header(&request, "Content-Length", 1, "80");
	assert_body(&request, "{\n"
						  "\t\"foo\": \"bar\",\n"
						  "\t\"baz\": 42,\n"
						  "\t\"blarg\": {\n"
						  "\t\t\"array\": [\n"
						  "\t\t\t1,\n"
						  "\t\t\t2,\n"
						  "\t\t\t3\n"
						  "\t\t]\n"
						  "\t}\n"
						  "}\n");
	http_request_dealloc(&request);
}

void parse_request_put_no_body() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "PUT /1/2/3 HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "foo: bar\r\n"
										 "foo: baz\r\n"
										 "\r\n");
	assert_method(&request, "PUT");
	assert_uri(&request, "/1/2/3");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_header(&request, "foo", 2, "bar", "baz");
	assert_no_body(&request);
	http_request_dealloc(&request);
}

void parse_request_put_with_body_text() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "PUT /1/2/3 HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "Content-Length: 9\r\n"
										 "Content-Type: application/x-www-form-urlencoded\r\n"
										 "something: value1,value2\r\n"
										 "something-else: value3\r\n"
										 "something: value4\r\n"
										 "something: value5,value6,value7,value8,value9\r\n"
										 "\r\n"
										 "some data");
	assert_method(&request, "PUT");
	assert_uri(&request, "/1/2/3");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_header(&request, "Content-Type", 1, "application/x-www-form-urlencoded");
	assert_header(&request, "Content-Length", 1, "9");
	assert_header(&request, "something", 8, "value1", "value2", "value4", "value5", "value6", "value7", "value8", "value9");
	assert_header(&request, "something-else", 1, "value3");
	assert_body(&request, "some data");
	http_request_dealloc(&request);
}

void parse_request_delete_no_body() {
	http_request request;
	http_request_init(&request);
	assert_parses_successfully(&request, "DELETE /1/2/3 HTTP/1.1\r\n"
										 "Host: example.com\r\n"
										 "User-Agent: curl/7.68.0\r\n"
										 "Accept: */*\r\n"
										 "\r\n");
	assert_method(&request, "DELETE");
	assert_uri(&request, "/1/2/3");
	assert_header(&request, "Host", 1, "example.com");
	assert_header(&request, "User-Agent", 1, "curl/7.68.0");
	assert_header(&request, "Accept", 1, "*/*");
	assert_no_body(&request);
	http_request_dealloc(&request);
}

void response_no_headers_no_body(char *expected_response, int status_code) {
	buffer b;
	stream s;
	buffer_init(&b);
	stream_init_buffer(&s, &b, 1);

	assert(http_response_write(&s, status_code, NULL) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);

	// now do the same thing with every variant, passing null for the body
	// this proves null checks work
	stream_set_position(&s, 0);
	assert(http_response_write_data(&s, status_code, NULL, NULL, 0) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);
	stream_set_position(&s, 0);
	assert(http_response_write_buffer(&s, status_code, NULL, NULL) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);
	stream_set_position(&s, 0);
	assert(http_response_write_str(&s, status_code, NULL, NULL) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);
	stream_set_position(&s, 0);
	assert(http_response_write_cstr(&s, status_code, NULL, NULL) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);
	stream_set_position(&s, 0);
	assert(http_response_write_stream(&s, status_code, NULL, NULL) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);

	stream_dealloc(&s, NULL);
}

void response_headers_no_body(char *expected_response, int status_code, size_t num_headers, ...) {
	buffer b;
	stream s;
	buffer_init(&b);
	stream_init_buffer(&s, &b, 1);

	http_headers headers;
	http_headers_init(&headers);

	http_headers_clear(&headers);
	va_list args;
	va_start(args, num_headers);
	for (size_t i = 0; i < num_headers; i++) {
		http_header *header = http_headers_get_cstr(&headers, va_arg(args, char *), 1);
		string *value = http_header_append_value(header);
		string_set_cstr(value, va_arg(args, char *));
	}
	va_end(args);

	assert(http_response_write(&s, status_code, &headers) == 0);
	assert(buffer_get_length(&b) == strlen(expected_response));
	assert(memcmp(b.data, expected_response, strlen(expected_response)) == 0);

	http_headers_dealloc(&headers);

	stream_dealloc(&s, NULL);
}

// TODO JEFF tests with body

int main() {
	header();
	headers();
	parse_request_get();
	parse_request_post_no_body();
	parse_request_post_with_body_text();
	parse_request_post_with_body_json();
	parse_request_put_no_body();
	parse_request_put_with_body_text();
	parse_request_delete_no_body();
	response_no_headers_no_body("HTTP/1.1 200 OK\r\n", 200);
	response_headers_no_body("HTTP/1.1 404 Not Found\r\n", 404, 1, "foo", "bar");
	// TODO JEFF more versions of response testing
	return 0;
}