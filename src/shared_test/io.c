#include <assert.h>
#include <errno.h>
#include <string.h>

#include "../shared/io.h"

// TODO JEFF file test

// TODO JEFF socket test

void buffer_test_ptr() {
	buffer b;
	buffer_init(&b);

	buffer_append_bytes(&b, "Hello, World!", 13);

	io io;
	io_init_buffer(&io, &b, 1);

	void *dst = malloc(1024);

	assert(io_read(&io, dst, 3, NULL) == 3);
	assert(!memcmp(dst, "Hel", 3));
	assert(io_read(&io, dst, 3, NULL) == 3);
	assert(!memcmp(dst, "Hello,", 6));
	assert(io_read(&io, dst, 20, NULL) == 7);
	assert(!memcmp(dst, "Hello, World!", 13));
	assert(io_read(&io, dst, 20, NULL) == 0);
	assert(!memcmp(dst, "Hello, World!", 13));

	free(dst);
	io_dealloc(&io, NULL);
}

void buffer_test_buffer() {
	buffer b, dst_b;
	buffer_init(&b);
	buffer_init(&dst_b);

	buffer_append_bytes(&b, "Hello, World!", 13);

	io io;
	io_init_buffer(&io, &b, 0);

	assert(io_read_buffer(&io, &dst_b, 3, NULL) == 3);
	assert(buffer_get_length(&dst_b) == 3);
	assert(!memcmp(dst_b.data, "Hel", 3));
	assert(io_read_buffer(&io, &dst_b, 3, NULL) == 3);
	assert(buffer_get_length(&dst_b) == 6);
	assert(!memcmp(dst_b.data, "Hello,", 6));
	assert(io_read_buffer(&io, &dst_b, 20, NULL) == 7);
	assert(buffer_get_length(&dst_b) == 13);
	assert(!memcmp(dst_b.data, "Hello, World!", 13));
	assert(io_read_buffer(&io, &dst_b, 20, NULL) == 0);
	assert(buffer_get_length(&dst_b) == 13);
	assert(!memcmp(dst_b.data, "Hello, World!", 13));

	buffer_dealloc(&b);
	buffer_dealloc(&dst_b);
	io_dealloc(&io, NULL);
}

int main() {
	buffer_test_ptr();
	buffer_test_buffer();
	return 0;
}