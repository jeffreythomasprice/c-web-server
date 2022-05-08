/*
This test is going to write data to a socket, and have the receive callback signal a semaphore that data is available. Every time that
signal is received the main thread will check the received data against what it sent.

The data sent is prefixed with a 2-byte length.
*/

#include "../shared/tcp_socket_wrapper.h"
#include "../shared/log.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void get_sockaddr_info_4(uint32_t actual_addr, uint16_t actual_port, char *expected_addr, uint16_t expected_port) {
	struct sockaddr saddr;
	((struct sockaddr_in *)&saddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&saddr)->sin_addr.s_addr = actual_addr;
	((struct sockaddr_in *)&saddr)->sin_port = actual_port;
	string addr;
	string_init(&addr);
	uint16_t port;
	assert(get_sockaddr_info_str(&saddr, &addr, &port) == 0);
	assert(string_compare_cstr(&addr, expected_addr, STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(port == expected_port);
	string_dealloc(&addr);
}

void get_sockaddr_info_6(uint8_t actual_addr[16], uint16_t actual_port, char *expected_addr, uint16_t expected_port) {
	struct sockaddr saddr;
	((struct sockaddr_in6 *)&saddr)->sin6_family = AF_INET6;
	memcpy(&((struct sockaddr_in6 *)&saddr)->sin6_addr, actual_addr, 16);
	((struct sockaddr_in6 *)&saddr)->sin6_port = actual_port;
	string addr;
	string_init(&addr);
	uint16_t port;
	assert(get_sockaddr_info_str(&saddr, &addr, &port) == 0);
	assert(string_compare_cstr(&addr, expected_addr, STRING_COMPARE_CASE_SENSITIVE) == 0);
	assert(port == expected_port);
	string_dealloc(&addr);
}

void socket_accept(void *data, string *address, uint16_t port, int socket) {
	assert(*((int *)data) == 42);

	// read the length header
	uint16_t len;
	int result = read(socket, &len, 2);
	if (result != 2) {
		if (result < 0) {
			log_error("receiving request data, error reading length for packet %i\n", errno);
		} else {
			log_error("receiving request data, expected to read 2 bytes, got %i\n", result);
		}
		close(socket);
		return;
	}
	// convert to local byte order
	len = ntohs(len);
	log_trace("request length %i\n", (int)len);
	// read the actual data
	uint8_t *buffer = malloc(len);
	result = read(socket, buffer, len);
	if (result != len) {
		if (result < 0) {
			log_error("receiving request data, error reading data for packet %i\n", errno);
		} else {
			log_error("receiving request data, expected to read %i bytes, got %i\n", (int)len, result);
		}
		free(buffer);
		close(socket);
		return;
	}
	// modify data for test
	for (int i = 0; i < len; i++) {
		buffer[i] = buffer[i] + 1;
	}
	// send the data back to the caller, including the length
	len = htons(len);
	result = write(socket, &len, 2);
	len = ntohs(len);
	if (result != 2) {
		if (result < 0) {
			log_error("sending response data, error writing length for packet %i\n", errno);
		} else {
			log_error("sending response data, expected to write 2 bytes, got %i\n", result);
		}
		free(buffer);
		close(socket);
		return;
	}
	result = write(socket, buffer, len);
	if (result != len) {
		if (result < 0) {
			log_error("sending response data, error writing data for packet %i\n", errno);
		} else {
			log_error("sending response data, expected to write %i bytes, got %i\n", (int)len, result);
		}
		free(buffer);
		close(socket);
		return;
	}
	free(buffer);
	close(socket);
}

int send_test_data() {
	uint16_t test_data_len = rand() % 1000 + 1000;
	log_trace("generating %i bytes of test data\n", (int)test_data_len);
	uint8_t *test_data = malloc(test_data_len);
	uint8_t *expected_response_data = malloc(test_data_len);
	for (int i = 0; i < test_data_len; i++) {
		test_data[i] = rand() % 256;
		expected_response_data[i] = test_data[i] + 1;
	}

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		log_error("error making client socket, %s\n", strerror(errno));
		free(test_data);
		return 1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(8000);
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr))) {
		log_error("error connecting client socket, %s\n", strerror(errno));
		free(test_data);
		return 1;
	}

	test_data_len = htons(test_data_len);
	int result = write(s, &test_data_len, 2);
	test_data_len = ntohs(test_data_len);
	if (result != 2) {
		if (result < 0) {
			log_error("sending request data, error writing length for packet %i\n", errno);
		} else {
			log_error("sending request data, expected to write 2 bytes, got %i\n", result);
		}
		close(s);
		free(test_data);
		return 1;
	}

	result = write(s, test_data, test_data_len);
	if (result != test_data_len) {
		if (result < 0) {
			log_error("sending request data, error writing data for packet %i\n", errno);
		} else {
			log_error("sending request data, expected to write %i bytes, got %i\n", (int)test_data_len, result);
		}
		close(s);
		free(test_data);
		return 1;
	}

	uint16_t receive_len;
	result = read(s, &receive_len, 2);
	if (result != 2) {
		if (result < 0) {
			log_error("receiving response data, error reading length for packet %i\n", errno);
		} else {
			log_error("receiving response data, expected to read 2 bytes, got %i\n", result);
		}
		close(s);
		free(test_data);
		return 1;
	}
	receive_len = ntohs(receive_len);
	log_trace("response length %i\n", (int)receive_len);

	uint8_t *receive_data = malloc(receive_len);
	result = read(s, receive_data, receive_len);
	if (result != receive_len) {
		if (result < 0) {
			log_error("receiving response data, error reading length for packet %i\n", errno);
		} else {
			log_error("receiving response data, expected to read %i bytes, got %i\n", (int)receive_len, result);
		}
		free(receive_data);
		close(s);
		free(test_data);
		return 1;
	}

	assert(receive_len == test_data_len);
	assert(!memcmp(receive_data, expected_response_data, test_data_len));

	free(receive_data);
	close(s);
	free(test_data);
	free(expected_response_data);
	return 0;
}

void do_socket_test(char *address, uint16_t port) {
	int data = 42;
	tcp_socket_wrapper sock_wrap;
	assert(tcp_socket_wrapper_init(&sock_wrap, address, port, socket_accept, &data) == 0);
	assert(send_test_data() == 0);
	assert(tcp_socket_wrapper_dealloc(&sock_wrap) == 0);
}

int main() {
	srand(time(NULL));

	get_sockaddr_info_4(htonl(0x01020304), htons(0x5678), "1.2.3.4", 0x5678);
	get_sockaddr_info_4(htonl(0x00000000), htons(0x0000), "0.0.0.0", 0x0000);
	get_sockaddr_info_4(htonl(0xfeffffff), htons(0x1234), "254.255.255.255", 0x1234);
	static uint8_t example_ipv6_1[16] = {0x20, 0x01, 0x08, 0x88, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
	get_sockaddr_info_6(example_ipv6_1, htons(0x7890), "2001:888:0:2::2", 0x7890);
	static uint8_t example_ipv6_2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	get_sockaddr_info_6(example_ipv6_2, htons(0x7890), "::", 0x7890);
	static uint8_t example_ipv6_3[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
	get_sockaddr_info_6(example_ipv6_3, htons(0x7890), "::1", 0x7890);

	string hostname, address;
	string_init(&hostname);
	string_init(&address);
	assert(get_hostname_str(&hostname) == 0);
	assert(get_address_for_hostname_str(&hostname, &address) == 0);

	do_socket_test(NULL, 8000);
	do_socket_test("0.0.0.0", 8000);
	// TODO JEFF tests for address parsing
	// do_socket_test("127.0.0.1", 8000);
	// do_socket_test(string_get_cstr(&hostname), 8000);
	// do_socket_test(string_get_cstr(&address), 8000);
	// do_socket_test("::", 8000);
	// do_socket_test("::1", 8000);

	string_dealloc(&hostname);
	string_dealloc(&address);

	return 0;
}