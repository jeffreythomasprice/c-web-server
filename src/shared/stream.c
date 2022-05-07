#include "stream.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

int stream_file_descriptor_read(stream *stream, void *dst, size_t n, string *error) {
	size_t result = read(stream->file_descriptor.file_descriptor, dst, n);
	if (result < 0 && error) {
		string_set_cstrf(error, "error reading from file descriptor: %s", strerror(errno));
	}
	return result;
}

int stream_file_descriptor_write(stream *stream, void *dst, size_t n, string *error) {
	size_t result = write(stream->file_descriptor.file_descriptor, dst, n);
	if (result < 0 && error) {
		string_set_cstrf(error, "error writing to file descriptor: %s", strerror(errno));
	}
	return result;
}

size_t stream_file_descriptor_get_position(stream *stream) {
	return lseek(stream->file_descriptor.file_descriptor, 0, SEEK_CUR);
}

size_t stream_file_descriptor_set_position(stream *stream, size_t pos) {
	lseek(stream->file_descriptor.file_descriptor, pos, SEEK_SET);
	return lseek(stream->file_descriptor.file_descriptor, 0, SEEK_CUR);
}

size_t stream_file_descriptor_get_length(stream *stream) {
	// current position
	size_t pos = lseek(stream->file_descriptor.file_descriptor, 0, SEEK_CUR);
	// go to end, returns offset from beginning, so the length
	size_t result = lseek(stream->file_descriptor.file_descriptor, 0, SEEK_END);
	// restore original position
	lseek(stream->file_descriptor.file_descriptor, pos, SEEK_SET);
	return result;
}

int stream_file_descriptor_close(stream *stream, string *error) {
	if (stream->file_descriptor.should_close) {
		close(stream->file_descriptor.file_descriptor);
	}
}

int stream_buffer_read(stream *stream, void *dst, size_t n, string *error) {
	size_t remaining = buffer_get_length(stream->buffer.buffer) - stream->buffer.index;
	if (remaining < n) {
		n = remaining;
	}
	memcpy(dst, stream->buffer.buffer->data + stream->buffer.index, n);
	stream->buffer.index += n;
	return n;
}

int stream_buffer_write(stream *stream, void *src, size_t n, string *error) {
	size_t needs_at_least_length = stream->buffer.index + n;
	size_t cur_len = buffer_get_length(stream->buffer.buffer);
	if (needs_at_least_length > cur_len) {
		buffer_set_length(stream->buffer.buffer, needs_at_least_length);
	}
	memcpy(stream->buffer.buffer->data + stream->buffer.index, src, n);
	stream->buffer.index += n;
	return n;
}

size_t stream_buffer_get_position(stream *stream) {
	return stream->buffer.index;
}

size_t stream_buffer_set_position(stream *stream, size_t pos) {
	size_t len = buffer_get_length(stream->buffer.buffer);
	if (pos >= len) {
		pos = pos = len - 1;
	}
	stream->buffer.index = pos;
	return pos;
}

size_t stream_buffer_get_length(stream *stream) {
	return buffer_get_length(stream->buffer.buffer);
}

int stream_buffer_close(stream *stream, string *error) {
	if (stream->buffer.should_dealloc) {
		buffer_dealloc(stream->buffer.buffer);
	}
	return 0;
}

void stream_init_file_descriptor(stream *stream, int file_descriptor, int should_close) {
	stream->read = (stream_func_read)stream_file_descriptor_read;
	stream->write = (stream_func_write)stream_file_descriptor_write;
	stream->get_position = (stream_func_get_position)stream_file_descriptor_get_position;
	stream->set_position = (stream_func_set_position)stream_file_descriptor_set_position;
	stream->get_length = (stream_func_get_length)stream_file_descriptor_get_length;
	stream->close = (stream_func_close)stream_file_descriptor_close;
	stream->file_descriptor.file_descriptor = file_descriptor;
	stream->file_descriptor.should_close = should_close;
}

int stream_init_file_cstr(stream *stream, char *path, char *mode, string *error) {
	stream->read = (stream_func_read)stream_file_descriptor_read;
	stream->write = (stream_func_write)stream_file_descriptor_write;
	stream->get_position = (stream_func_get_position)stream_file_descriptor_get_position;
	stream->set_position = (stream_func_set_position)stream_file_descriptor_set_position;
	stream->get_length = (stream_func_get_length)stream_file_descriptor_get_length;
	stream->close = (stream_func_close)stream_file_descriptor_close;
	FILE *file = fopen(path, mode);
	if (!file) {
		fflush(stdout);
		if (error) {
			string_set_cstrf(error, "error opening file at \"%s\" with mode \"%s\": %s", path, mode, strerror(errno));
		}
		return 1;
	}
	stream->file_descriptor.file_descriptor = fileno(file);
	stream->file_descriptor.should_close = 1;
	return 0;
}

void stream_init_buffer(stream *stream, buffer *buffer, int should_dealloc) {
	stream->read = (stream_func_read)stream_buffer_read;
	stream->write = (stream_func_write)stream_buffer_write;
	stream->get_position = (stream_func_get_position)stream_buffer_get_position;
	stream->set_position = (stream_func_set_position)stream_buffer_set_position;
	stream->get_length = (stream_func_get_length)stream_buffer_get_length;
	stream->close = (stream_func_close)stream_buffer_close;
	stream->buffer.buffer = buffer;
	stream->buffer.should_dealloc = should_dealloc;
	stream->buffer.index = 0;
}

int stream_dealloc(stream *stream, string *error) {
	return stream->close(stream, error);
}

int stream_read(stream *stream, void *dst, size_t n, string *error) {
	return stream->read(stream, dst, n, error);
}

int stream_write(stream *stream, void *src, size_t n, string *error) {
	return stream->write(stream, src, n, error);
}

size_t stream_get_position(stream *stream) {
	return stream->get_position(stream);
}

size_t stream_set_position(stream *stream, size_t pos) {
	return stream->set_position(stream, pos);
}

size_t stream_get_length(stream *stream) {
	return stream->get_length(stream);
}

int stream_read_buffer(stream *stream, buffer *dst, size_t n, string *error) {
	buffer_ensure_capacity(dst, buffer_get_length(dst) + n);
	int result = stream_read(stream, dst->data + buffer_get_length(dst), n, error);
	if (result > 0) {
		buffer_set_length(dst, buffer_get_length(dst) + result);
	}
	return result;
}