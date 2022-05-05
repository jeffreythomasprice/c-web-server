#ifndef io_h
#define io_h

#include <stdio.h>

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO refactor, this should be streams, not io

typedef int (*io_func_read)(void *io, void *dst, size_t n, string *error);
typedef int (*io_func_close)(void *io, string *error);

typedef struct {
	io_func_read read;
	// TODO needs write functionality
	io_func_close close;
	union {
		struct {
			int file_descriptor;
			int should_close;
		} file_descriptor;
		struct {
			buffer *buffer;
			int should_dealloc;
			size_t index;
		} buffer;
	};
} io;

void io_init_file_descriptor(io *io, int file_descriptor, int should_close);
/**
 * Calls fopen.
 * @param error optional, if provided and an error occurs the contents are replaced with an error message
 */
int io_init_file_cstr(io *io, char *path, char *mode, string *error);
void io_init_buffer(io *io, buffer *buffer, int should_dealloc);
int io_dealloc(io *io, string *error);
/**
 * @param dst the destination to write the result to
 * @param n the number of bytes to read
 * @param error optional, if provided and an error occurs the contents are replaced with an error message
 * @returns as per fread or read, positive values indicate bytes read, 0 indicates no bytes available, negative values indicate errors
 */
int io_read(io *io, void *dst, size_t n, string *error);
/**
 * As io_read, but reads into the given buffer. Expands the buffer as needed to fill in new data.
 */
int io_read_buffer(io *io, buffer *dst, size_t n, string *error);

#ifdef __cplusplus
}
#endif

#endif