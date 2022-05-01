#include "io.h"

#include <errno.h>
#include <string.h>

typedef struct {
	io io;
	FILE *file;
	int should_close;
} io_file;

typedef struct {
	io io;
	int socket;
	int should_close;
} io_socket;

typedef struct {
	io io;
	buffer *buffer;
	int should_dealloc;
} io_buffer;

int io_file_read(void *io, void *dst, size_t n, string *error);
int io_file_close(void *io, string *error);
io_vtable io_vtable_file = {io_file_read, io_file_close};

int io_socket_read(void *io, void *dst, size_t n, string *error);
int io_socket_close(void *io, string *error);
io_vtable io_vtable_socket = {io_socket_read, io_socket_close};

int io_buffer_read(void *io, void *dst, size_t n, string *error);
int io_buffer_close(void *io, string *error);
io_vtable io_vtable_buffer = {io_buffer_read, io_buffer_close};

int io_file_read(void *io, void *dst, size_t n, string *error) {
	io_file *fio = io;
	size_t result = fread(dst, 1, n, fio->file);
	if (result != n) {
		int error_code = ferror(fio->file);
		if (error_code) {
			string_set_cstrf(error, "error reading from file: %s", strerror(error_code));
		}
		return -1;
	}
	return result;
}

int io_file_close(void *io, string *error) {
	io_file *fio = io;
	if (fio->should_close) {
		fclose(fio->file);
	}
}

int io_socket_read(void *io, void *dst, size_t n, string *error) {
	// TODO JEFF implement me!
}

int io_socket_close(void *io, string *error) {
	// TODO JEFF implement me!
}

int io_buffer_read(void *io, void *dst, size_t n, string *error) {
	// TODO JEFF implement me!
}

int io_buffer_close(void *io, string *error) {
	// TODO JEFF implement me!
}

void io_init_file(io *io, FILE *file, int should_close) {
	io_file *fio = (io_file *)io;
	fio->io.vtable = io_vtable_file;
	fio->file = file;
	fio->should_close = should_close;
}

int io_init_file_cstr(io *io, char *path, char *mode, string *error) {
	io_file *fio = (io_file *)io;
	fio->io.vtable = io_vtable_file;
	fio->file = fopen(path, mode);
	if (!fio->file) {
		if (error) {
			string_set_cstrf(error, "error opening file at \"%s\" with mode \"%s\": %s", path, mode, strerror(errno));
		}
		return 1;
	}
	fio->should_close = 1;
	return 0;
}

void io_init_socket(io *io, int socket, int should_close) {
	io_socket *sio = (io_socket *)io;
	sio->io.vtable = io_vtable_socket;
	sio->socket = socket;
	sio->should_close = should_close;
}

void io_init_buffer(io *io, buffer *buffer, int should_dealloc) {
	io_buffer *bio = (io_buffer *)io;
	bio->io.vtable = io_vtable_buffer;
	bio->buffer = buffer;
	bio->should_dealloc = should_dealloc;
}

int io_dealloc(io *io, string *error) {
	return io->vtable.close(io, error);
}

int io_read(io *io, void *dst, size_t n, string *error) {
	return io->vtable.read(io, dst, n, error);
}

int io_read_buffer(io *io, buffer *dst, size_t n, string *error) {
	buffer_ensure_capacity(dst, buffer_get_length(dst) + n);
	int result = io_read(io, dst->data + buffer_get_length(dst), n, error);
	if (result > 0) {
		buffer_set_length(dst, buffer_get_length(dst) + result);
	}
	return result;
}