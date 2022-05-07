#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../shared/stream.h"

void hello_world_test(stream *stream) {
	assert(stream_get_position(stream) == 0);
	assert(stream_get_length(stream) == 0);

	assert(stream_write(stream, "Hello, World!", 13, NULL) == 13);
	assert(stream_get_position(stream) == 13);
	assert(stream_get_length(stream) == 13);

	assert(stream_set_position(stream, 0) == 0);
	assert(stream_get_position(stream) == 0);

	void *dst = malloc(1024);
	void *dst_ptr = dst;
	assert(stream_read(stream, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hel", 3));
	dst_ptr += 3;
	assert(stream_read(stream, dst_ptr, 3, NULL) == 3);
	assert(!memcmp(dst, "Hello,", 6));
	dst_ptr += 3;
	assert(stream_read(stream, dst_ptr, 20, NULL) == 7);
	assert(!memcmp(dst, "Hello, World!", 13));
	dst_ptr += 7;
	assert(stream_read(stream, dst_ptr, 20, NULL) == 0);
	assert(!memcmp(dst, "Hello, World!", 13));
	free(dst);

	assert(stream_get_position(stream) == 13);
	assert(stream_set_position(stream, 0) == 0);
	assert(stream_get_position(stream) == 0);

	buffer dst_b;
	buffer_init(&dst_b);
	assert(stream_read_buffer(stream, &dst_b, 3, NULL) == 3);
	assert(buffer_get_length(&dst_b) == 3);
	assert(!memcmp(dst_b.data, "Hel", 3));
	assert(stream_read_buffer(stream, &dst_b, 3, NULL) == 3);
	assert(buffer_get_length(&dst_b) == 6);
	assert(!memcmp(dst_b.data, "Hello,", 6));
	assert(stream_read_buffer(stream, &dst_b, 20, NULL) == 7);
	assert(buffer_get_length(&dst_b) == 13);
	assert(!memcmp(dst_b.data, "Hello, World!", 13));
	assert(stream_read_buffer(stream, &dst_b, 20, NULL) == 0);
	assert(buffer_get_length(&dst_b) == 13);
	assert(!memcmp(dst_b.data, "Hello, World!", 13));
	buffer_dealloc(&dst_b);

	assert(stream_get_position(stream) == 13);
}

void file_descriptor_test() {
	string path;
	string_init(&path);
	string_append_cstrf(&path, "%s/filetest-XXXXXX", P_tmpdir);
	int file_descriptor = mkstemp(string_get_cstr(&path));

	stream stream;
	stream_init_file_descriptor(&stream, file_descriptor, 1);

	hello_world_test(&stream);

	stream_dealloc(&stream, NULL);
	unlink(string_get_cstr(&path));
	string_dealloc(&path);
}

void file_name_test() {
	string path;
	string_init(&path);
	string_append_cstrf(&path, "%s/filetest-XXXXXX", P_tmpdir);
	int file_descriptor = mkstemp(string_get_cstr(&path));

	stream stream;
	assert(stream_init_file_cstr(&stream, string_get_cstr(&path), "w+", NULL) == 0);

	hello_world_test(&stream);

	stream_dealloc(&stream, NULL);
	unlink(string_get_cstr(&path));
	string_dealloc(&path);
}

void buffer_test() {
	buffer b;
	buffer_init(&b);

	stream stream;
	stream_init_buffer(&stream, &b, 1);

	hello_world_test(&stream);

	stream_dealloc(&stream, NULL);
}

void buffered_reader_test() {
	buffer input_buffer;
	buffer_init(&input_buffer);
	stream input_stream;
	stream_init_buffer(&input_stream, &input_buffer, 1);

	stream_write(&input_stream, "Hello, World!", 13, NULL);
	stream_set_position(&input_stream, 0);

	stream stream;
	stream_init_buffered_reader(&stream, &input_stream, 1);

	assert(buffer_get_length(&stream.buffered_reader.buffer) == 0);
	assert(input_stream.buffer.position == 0);
	assert(stream_get_position(&stream) == 0);
	assert(stream_get_length(&stream) == 0);

	buffer dst;
	buffer_init(&dst);

	assert(stream_read_buffer(&stream, &dst, 3, NULL) == 3);
	assert(buffer_get_length(&stream.buffered_reader.buffer) == 3);
	assert(input_stream.buffer.position == 3);
	assert(buffer_get_length(&dst) == 3);
	assert(memcmp(dst.data, "Hel", 3) == 0);

	assert(stream_read_buffer(&stream, &dst, 4, NULL) == 4);
	assert(buffer_get_length(&stream.buffered_reader.buffer) == 7);
	assert(input_stream.buffer.position == 7);
	assert(buffer_get_length(&dst) == 7);
	assert(memcmp(dst.data, "Hello, ", 7) == 0);

	assert(stream_set_position(&stream, 4) == 4);
	buffer_set_length(&dst, 0);
	assert(stream_read_buffer(&stream, &dst, 7, NULL) == 7);
	assert(buffer_get_length(&stream.buffered_reader.buffer) == 11);
	assert(input_stream.buffer.position == 11);
	assert(buffer_get_length(&dst) == 7);
	assert(memcmp(dst.data, "o, Worl", 7) == 0);

	assert(stream_read_buffer(&stream, &dst, 20, NULL) == 2);
	assert(buffer_get_length(&stream.buffered_reader.buffer) == 13);
	assert(input_stream.buffer.position == 13);
	assert(buffer_get_length(&dst) == 9);
	assert(memcmp(dst.data, "o, World!", 9) == 0);

	assert(stream_read_buffer(&stream, &dst, 20, NULL) == 0);
	assert(buffer_get_length(&stream.buffered_reader.buffer) == 13);
	assert(input_stream.buffer.position == 13);
	assert(buffer_get_length(&dst) == 9);
	assert(memcmp(dst.data, "o, World!", 9) == 0);

	assert(stream_set_position(&stream, 999) == 13);
	assert(stream_set_position(&stream, 2) == 2);
	buffer_set_length(&dst, 0);
	assert(stream_read_buffer(&stream, &dst, 20, NULL) == 11);
	assert(buffer_get_length(&stream.buffered_reader.buffer) == 13);
	assert(input_stream.buffer.position == 13);
	assert(memcmp(dst.data, "llo, World!", 13) == 0);

	assert(stream_set_position(&stream, 0) == 0);
	assert(stream_write(&stream, "foo", 3, NULL) == -1);

	buffer_dealloc(&dst);
	stream_dealloc(&stream, NULL);
}

int main() {
	file_descriptor_test();
	file_name_test();
	buffer_test();
	buffered_reader_test();
	return 0;
}