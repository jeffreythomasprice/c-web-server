#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

void read_all_test(size_t test_data_size, size_t expected_read_len, size_t max, size_t block_size) {
	buffer test_data, result;
	buffer_init(&test_data);
	buffer_init(&result);

	buffer_set_length(&test_data, test_data_size);
	for (size_t i = 0; i < test_data_size; i++) {
		test_data.data[i] = rand();
	}

	stream s;
	stream_init_buffer(&s, &test_data, 0);
	assert(stream_read_all_into_buffer(&s, &result, max, block_size, NULL) == expected_read_len);
	stream_dealloc(&s, NULL);

	assert(buffer_get_length(&result) == expected_read_len);
	assert(memcmp(result.data, test_data.data, expected_read_len) == 0);

	buffer_dealloc(&test_data);
	buffer_dealloc(&result);
}

void write_buffer() {
	buffer dst_buf;
	buffer_init(&dst_buf);
	stream dst;
	stream_init_buffer(&dst, &dst_buf, 1);

	buffer src_buf;
	buffer_init(&src_buf);
	buffer_append_bytes(&src_buf, "Hello, World!", 13);

	assert(stream_write_buffer(&dst, &src_buf, NULL) == 13);
	assert(memcmp(dst_buf.data, "Hello, World!", 13) == 0);

	// clear the buffer
	// this should catch the case where changing stream position when buffer length is 0, was a bug where this caused it to set position to
	// -1 breaking frees
	buffer_clear(&dst_buf);
	stream_set_position(&dst, 0);

	assert(stream_write_buffer(&dst, &src_buf, NULL) == 13);
	assert(memcmp(dst_buf.data, "Hello, World!", 13) == 0);

	buffer_dealloc(&src_buf);
	stream_dealloc(&dst, NULL);
}

void write_str() {
	buffer dst_buf;
	buffer_init(&dst_buf);
	stream dst;
	stream_init_buffer(&dst, &dst_buf, 1);

	string src;
	string_init_cstr(&src, "Hello, World!");

	assert(stream_write_str(&dst, &src, NULL) == 13);
	assert(memcmp(dst_buf.data, "Hello, World!", 13) == 0);

	string_dealloc(&src);
	stream_dealloc(&dst, NULL);
}

void write_cstr() {
	buffer dst_buf;
	buffer_init(&dst_buf);
	stream dst;
	stream_init_buffer(&dst, &dst_buf, 1);

	assert(stream_write_cstr(&dst, "Hello, World!", NULL) == 13);
	assert(memcmp(dst_buf.data, "Hello, World!", 13) == 0);

	stream_dealloc(&dst, NULL);
}

void write_cstrf() {
	buffer dst_buf;
	buffer_init(&dst_buf);
	stream dst;
	stream_init_buffer(&dst, &dst_buf, 1);

	assert(stream_write_cstrf(&dst, NULL, "foo %i bar %s", 42, "baz") == 14);
	assert(memcmp(dst_buf.data, "foo 42 bar baz", 14) == 0);

	stream_dealloc(&dst, NULL);
}

int main() {
	srand(time(NULL));
	file_descriptor_test();
	file_name_test();
	buffer_test();
	buffered_reader_test();
	read_all_test(1024, 1024, 0, 1024);
	read_all_test(512, 512, 0, 1024);
	read_all_test(32768, 32768, 0, 1024);
	read_all_test(32768, 2000, 2000, 1024);
	read_all_test(1500, 1500, 0, 1024);
	read_all_test(20, 20, 0, 1);
	write_buffer();
	write_str();
	write_cstr();
	write_cstrf();
	return 0;
}