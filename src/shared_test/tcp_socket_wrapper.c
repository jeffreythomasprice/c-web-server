#include "../shared/tcp_socket_wrapper.h"

#include <stdio.h>

void socket_accept(int s) {
	printf("TODO JEFF actually hand this off to a thread pool %i\n", s);
	close(s);
}

int main() {
	tcp_socket_wrapper sock_wrap;
	// TODO version of this test that tests binding only to localhost
	if (tcp_socket_wrapper_init(&sock_wrap, NULL, 8000, socket_accept)) {
		return 1;
	}

	printf("TODO JEFF implement test\n");
	/*
	want to send some bytes and prove they got read
	*/

	if (tcp_socket_wrapper_destroy(&sock_wrap)) {
		return 1;
	}
	return 0;
}