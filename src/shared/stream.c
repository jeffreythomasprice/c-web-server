#include "stream.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

int stream_file_descriptor_close(stream *stream, string *error) {
	if (stream->file_descriptor.should_close) {
		close(stream->file_descriptor.file_descriptor);
	}
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

int stream_buffer_close(stream *stream, string *error) {
	if (stream->buffer.should_dealloc) {
		buffer_dealloc(stream->buffer.buffer);
	}
	return 0;
}

size_t stream_buffer_get_position(stream *stream) {
	return stream->buffer.position;
}

size_t stream_buffer_set_position(stream *stream, size_t pos) {
	size_t len = buffer_get_length(stream->buffer.buffer);
	if (pos >= len) {
		pos = pos = len - 1;
	}
	stream->buffer.position = pos;
	return pos;
}

size_t stream_buffer_get_length(stream *stream) {
	return buffer_get_length(stream->buffer.buffer);
}

int stream_buffer_read(stream *stream, void *dst, size_t n, string *error) {
	size_t remaining = buffer_get_length(stream->buffer.buffer) - stream->buffer.position;
	if (remaining < n) {
		n = remaining;
	}
	memcpy(dst, stream->buffer.buffer->data + stream->buffer.position, n);
	stream->buffer.position += n;
	return n;
}

int stream_buffer_write(stream *stream, void *src, size_t n, string *error) {
	size_t needs_at_least_length = stream->buffer.position + n;
	size_t cur_len = buffer_get_length(stream->buffer.buffer);
	if (needs_at_least_length > cur_len) {
		buffer_set_length(stream->buffer.buffer, needs_at_least_length);
	}
	memcpy(stream->buffer.buffer->data + stream->buffer.position, src, n);
	stream->buffer.position += n;
	return n;
}

int stream_buffered_reader_close(stream *stream, string *error) {
	if (stream->buffered_reader.should_dealloc) {
		return stream_dealloc(stream->buffered_reader.input, error);
	}
	return 0;
}

size_t stream_buffered_reader_get_position(stream *stream) {
	return stream->buffered_reader.position;
}

size_t stream_buffered_reader_set_position(stream *stream, size_t pos) {
	size_t len = buffer_get_length(&stream->buffered_reader.buffer);
	if (pos > len) {
		pos = len;
	}
	stream->buffered_reader.position = pos;
	return pos;
}

size_t stream_buffered_reader_get_length(stream *stream) {
	return buffer_get_length(&stream->buffered_reader.buffer);
}

int stream_buffered_reader_read(stream *stream, void *dst, size_t n, string *error) {
	// how much is available in the buffer right now?
	size_t available = buffer_get_length(&stream->buffered_reader.buffer) - stream->buffered_reader.position;
	// less than we want, try to read more
	if (available < n) {
		size_t desired = n - available;
		size_t result = stream_read_buffer(stream->buffered_reader.input, &stream->buffered_reader.buffer, desired, error);
		if (result < 0) {
			return result;
		}
		// more is now available
		available += result;
	}
	// if we still have less than we want we must have hit the end of the input stream, so adjust how much we're copying
	if (available < n) {
		n = available;
	}
	memcpy(dst, stream->buffered_reader.buffer.data + stream->buffered_reader.position, n);
	stream->buffered_reader.position += n;
	return n;
}

int stream_buffered_reader_write(stream *stream, void *src, size_t n, string *error) {
	if (error) {
		string_set_cstr(error, "buffered reader streams aren't writable");
	}
	return -1;
}

void stream_init_file_descriptor(stream *stream, int file_descriptor, int should_close) {
	stream->close = (stream_func_close)stream_file_descriptor_close;
	stream->get_position = (stream_func_get_position)stream_file_descriptor_get_position;
	stream->set_position = (stream_func_set_position)stream_file_descriptor_set_position;
	stream->get_length = (stream_func_get_length)stream_file_descriptor_get_length;
	stream->read = (stream_func_read)stream_file_descriptor_read;
	stream->write = (stream_func_write)stream_file_descriptor_write;
	stream->file_descriptor.file_descriptor = file_descriptor;
	stream->file_descriptor.should_close = should_close;
}

int stream_init_file_cstr(stream *stream, char *path, char *mode, string *error) {
	stream->close = (stream_func_close)stream_file_descriptor_close;
	stream->get_position = (stream_func_get_position)stream_file_descriptor_get_position;
	stream->set_position = (stream_func_set_position)stream_file_descriptor_set_position;
	stream->get_length = (stream_func_get_length)stream_file_descriptor_get_length;
	stream->read = (stream_func_read)stream_file_descriptor_read;
	stream->write = (stream_func_write)stream_file_descriptor_write;
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
	stream->close = (stream_func_close)stream_buffer_close;
	stream->get_position = (stream_func_get_position)stream_buffer_get_position;
	stream->set_position = (stream_func_set_position)stream_buffer_set_position;
	stream->get_length = (stream_func_get_length)stream_buffer_get_length;
	stream->read = (stream_func_read)stream_buffer_read;
	stream->write = (stream_func_write)stream_buffer_write;
	stream->buffer.buffer = buffer;
	stream->buffer.should_dealloc = should_dealloc;
	stream->buffer.position = 0;
}

void stream_init_buffered_reader(stream *s, stream *input, int should_dealloc) {
	s->close = (stream_func_close)stream_buffered_reader_close;
	s->get_position = (stream_func_get_position)stream_buffered_reader_get_position;
	s->set_position = (stream_func_set_position)stream_buffered_reader_set_position;
	s->get_length = (stream_func_get_length)stream_buffered_reader_get_length;
	s->read = (stream_func_read)stream_buffered_reader_read;
	s->write = (stream_func_write)stream_buffered_reader_write;
	buffer_init(&s->buffered_reader.buffer);
	s->buffered_reader.position = 0;
	s->buffered_reader.input = input;
	s->buffered_reader.should_dealloc = should_dealloc;
}

int stream_dealloc(stream *stream, string *error) {
	return stream->close(stream, error);
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

int stream_read(stream *stream, void *dst, size_t n, string *error) {
	return stream->read(stream, dst, n, error);
}

int stream_write(stream *stream, void *src, size_t n, string *error) {
	return stream->write(stream, src, n, error);
}

int stream_read_buffer(stream *stream, buffer *dst, size_t n, string *error) {
	size_t len = buffer_get_length(dst);
	buffer_ensure_capacity(dst, len + n);
	int result = stream_read(stream, dst->data + len, n, error);
	if (result > 0) {
		buffer_set_length(dst, len + result);
	}
	return result;
}

int stream_read_all_into_buffer(stream *stream, buffer *dst, size_t max, size_t block_size, string *error) {
	size_t total = 0;
	while (1) {
		size_t next_read = block_size;
		if (max > 0 && total + next_read > max) {
			next_read = max - total;
		}
		if (next_read == 0) {
			break;
		}
		int result = stream_read_buffer(stream, dst, next_read, error);
		if (result < 0) {
			return result;
		}
		if (result == 0 || (max > 0 && total >= max)) {
			break;
		}
		total += result;
	}
	return total;
}
