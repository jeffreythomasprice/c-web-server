#include "io.h"

#include <errno.h>
#include <string.h>

int io_file_read(io *io, void *dst, size_t n, string *error) {
	size_t result = fread(dst, 1, n, io->file.file);
	if (result != n) {
		int error_code = ferror(io->file.file);
		if (error_code) {
			string_set_cstrf(error, "error reading from file: %s", strerror(error_code));
		}
		return -1;
	}
	return result;
}

int io_file_close(io *io, string *error) {
	if (io->file.should_close) {
		fclose(io->file.file);
	}
}

int io_socket_read(io *io, void *dst, size_t n, string *error) {
	// TODO JEFF implement me!
}

int io_socket_close(io *io, string *error) {
	// TODO JEFF implement me!
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

void io_init_file(io *io, FILE *file, int should_close) {
	io->read = (io_func_read)io_file_read;
	io->close = (io_func_close)io_file_close;
	io->file.file = file;
	io->file.should_close = should_close;
}

int io_init_file_cstr(io *io, char *path, char *mode, string *error) {
	io->read = (io_func_read)io_file_read;
	io->close = (io_func_close)io_file_close;
	io->file.file = fopen(path, mode);
	if (!io->file.file) {
		if (error) {
			string_set_cstrf(error, "error opening file at \"%s\" with mode \"%s\": %s", path, mode, strerror(errno));
		}
		return 1;
	}
	io->file.should_close = 1;
	return 0;
}

void io_init_socket(io *io, int socket, int should_close) {
	io->read = (io_func_read)io_socket_read;
	io->close = (io_func_close)io_socket_close;
	io->socket.socket = socket;
	io->socket.should_close = should_close;
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