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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void socket_accept(void *data, char *address, uint16_t port, int socket) {
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

int main() {
	srand(time(NULL));

	int data = 42;

	tcp_socket_wrapper sock_wrap;
	// TODO version of this test that tests binding only to localhost
	if (tcp_socket_wrapper_init(&sock_wrap, NULL, 8000, socket_accept, &data)) {
		return 1;
	}

	assert(send_test_data() == 0);

	if (tcp_socket_wrapper_dealloc(&sock_wrap)) {
		return 1;
	}
	return 0;
}