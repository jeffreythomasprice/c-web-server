#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../shared/io.h"

void file_descriptor_test() {
	string path;
	string_init(&path);
	string_append_cstrf(&path, "%s/filetest-XXXXXX", P_tmpdir);
	int file_descriptor = mkstemp(string_get_cstr(&path));
	unlink(string_get_cstr(&path));
	string_dealloc(&path);

	assert(write(file_descriptor, "Hello, World!", 13) == 13);
	assert(lseek(file_descriptor, 0, SEEK_SET) == 0);

	io io;
	io_init_file_descriptor(&io, file_descriptor, 1);

	void *dst = malloc(1024);
	void *dst_ptr = dst;
	assert(io_read(&io, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hel", 3));
	dst_ptr += 3;
	assert(io_read(&io, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hello,", 6));
	dst_ptr += 3;
	assert(io_read(&io, dst_ptr, 20, NULL) == 7);
	assert(!memcmp(dst, "Hello, World!", 13));
	dst_ptr += 7;
	assert(io_read(&io, dst_ptr, 20, NULL) == 0);
	assert(!memcmp(dst, "Hello, World!", 13));

	free(dst);
	io_dealloc(&io, NULL);
}

void file_name() {
	string path;
	string_init(&path);
	string_append_cstrf(&path, "%s/filetest-XXXXXX", P_tmpdir);
	int file_descriptor = mkstemp(string_get_cstr(&path));
	unlink(string_get_cstr(&path));
	string_dealloc(&path);

	assert(write(file_descriptor, "Hello, World!", 13) == 13);
	assert(lseek(file_descriptor, 0, SEEK_SET) == 0);

	io io;
	assert(io_init_file_cstr(&io, string_get_cstr(&path), "r", NULL) == 0);

	void *dst = malloc(1024);
	void *dst_ptr = dst;
	assert(io_read(&io, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hel", 3));
	dst_ptr += 3;
	assert(io_read(&io, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hello,", 6));
	dst_ptr += 3;
	assert(io_read(&io, dst_ptr, 20, NULL) == 7);
	assert(!memcmp(dst, "Hello, World!", 13));
	dst_ptr += 7;
	assert(io_read(&io, dst_ptr, 20, NULL) == 0);
	assert(!memcmp(dst, "Hello, World!", 13));

	free(dst);
	io_dealloc(&io, NULL);
}

void buffer_test_ptr() {
	buffer b;
	buffer_init(&b);

	buffer_append_bytes(&b, "Hello, World!", 13);

	io io;
	io_init_buffer(&io, &b, 1);

	void *dst = malloc(1024);
	void *dst_ptr = dst;
	assert(io_read(&io, dst, 3, NULL) == 3);
	assert(!memcmp(dst, "Hel", 3));
	dst_ptr += 3;
	assert(io_read(&io, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hello,", 6));
	dst_ptr += 3;
	assert(io_read(&io, dst_ptr, 20, NULL) == 7);
	assert(!memcmp(dst, "Hello, World!", 13));
	dst_ptr += 7;
	assert(io_read(&io, dst_ptr, 20, NULL) == 0);
	assert(!memcmp(dst, "Hello, World!", 13));

	free(dst);
	io_dealloc(&io, NULL);
}

void buffer_test_buffer() {
	buffer b;
	buffer_init(&b);

	buffer_append_bytes(&b, "Hello, World!", 13);

	io io;
	io_init_buffer(&io, &b, 0);

	buffer dst_b;
	buffer_init(&dst_b);
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

	buffer_dealloc(&dst_b);
	io_dealloc(&io, NULL);

	buffer_dealloc(&b);
}

int main() {
	file_descriptor_test();
	buffer_test_ptr();
	buffer_test_buffer();
	return 0;
}