#include "io.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

int io_file_descriptor_read(io *io, void *dst, size_t n, string *error) {
	size_t result = read(io->file_descriptor.file_descriptor, dst, n);
	if (result < 0 && error) {
		string_set_cstrf(error, "error reading from file descriptor: %s", strerror(errno));
	}
	return result;
}

int io_file_descriptor_close(io *io, string *error) {
	if (io->file_descriptor.should_close) {
		close(io->file_descriptor.file_descriptor);
	}
}

int io_buffer_read(io *io, void *dst, size_t n, string *error) {
	size_t remaining = buffer_get_length(io->buffer.buffer) - io->buffer.index;
	if (remaining < n) {
		n = remaining;
	}
	memcpy(dst, io->buffer.buffer->data + io->buffer.index, n);
	io->buffer.index += n;
	return n;
}

int io_buffer_close(io *io, string *error) {
	if (io->buffer.should_dealloc) {
		buffer_dealloc(io->buffer.buffer);
	}
	return 0;
}

void io_init_file_descriptor(io *io, int file_descriptor, int should_close) {
	io->read = (io_func_read)io_file_descriptor_read;
	io->close = (io_func_close)io_file_descriptor_close;
	io->file_descriptor.file_descriptor = file_descriptor;
	io->file_descriptor.should_close = should_close;
}

int io_init_file_cstr(io *io, char *path, char *mode, string *error) {
	io->read = (io_func_read)io_file_descriptor_read;
	io->close = (io_func_close)io_file_descriptor_close;
	FILE *file = fopen(path, mode);
	if (!file) {
		if (error) {
			string_set_cstrf(error, "error opening file at \"%s\" with mode \"%s\": %s", path, mode, strerror(errno));
		}
		return 1;
	}
	io->file_descriptor.file_descriptor = fileno(file);
	io->file_descriptor.should_close = 1;
	return 0;
}

void io_init_buffer(io *io, buffer *buffer, int should_dealloc) {
	io->read = (io_func_read)io_buffer_read;
	io->close = (io_func_close)io_buffer_close;
	io->buffer.buffer = buffer;
	io->buffer.should_dealloc = should_dealloc;
	io->buffer.index = 0;
}

int io_dealloc(io *io, string *error) {
	return io->close(io, error);
}

int io_read(io *io, void *dst, size_t n, string *error) {
	return io->read(io, dst, n, error);
}

int io_read_buffer(io *io, buffer *dst, size_t n, string *error) {
	buffer_ensure_capacity(dst, buffer_get_length(dst) + n);
	int result = io_read(io, dst->data + buffer_get_length(dst), n, error);
	if (result > 0) {
		buffer_set_length(dst, buffer_get_length(dst) + result);
	}
	return result;
}