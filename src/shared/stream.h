#ifndef io_h
#define io_h

#include <stdio.h>

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*stream_func_read)(void *stream, void *dst, size_t n, string *error);
typedef int (*stream_func_write)(void *stream, void *src, size_t n, string *error);
typedef size_t (*stream_func_get_position)(void *steam);
typedef size_t (*stream_func_set_position)(void *stream, size_t pos);
typedef size_t (*stream_func_get_length)(void *stream);
typedef int (*stream_func_close)(void *stream, string *error);

typedef struct stream {
	stream_func_close close;
	stream_func_get_position get_position;
	stream_func_set_position set_position;
	stream_func_get_length get_length;
	stream_func_read read;
	stream_func_write write;
	union {
		struct {
			int file_descriptor;
			int should_close;
		} file_descriptor;
		struct {
			buffer *buffer;
			int should_dealloc;
			size_t position;
		} buffer;
		struct {
			// owned here, will be dealloc
			buffer buffer;
			size_t position;
			// owned elsewhere, optional dealloc
			struct stream *input;
			int should_dealloc;
		} buffered_reader;
	};
} stream;

void stream_init_file_descriptor(stream *stream, int file_descriptor, int should_close);
/**
 * Calls fopen.
 * @param error optional, if provided and an error occurs the contents are replaced with an error message
 */
int stream_init_file_cstr(stream *stream, char *path, char *mode, string *error);
void stream_init_buffer(stream *stream, buffer *buffer, int should_dealloc);
void stream_init_buffered_reader(stream *s, stream *input, int should_dealloc);
int stream_dealloc(stream *stream, string *error);

/**
 * @return the offset from the start of the stream
 */
size_t stream_get_position(stream *stream);

/**
 * @param pos the new position in the stream
 * @return the actual new position in the stream, e.g. if the requested position is past the end of the stream
 */
size_t stream_set_position(stream *stream, size_t pos);

/**
 * @return the length of the stream
 */
size_t stream_get_length(stream *stream);

/**
 * @param dst the destination to write the result to
 * @param n the number of bytes to read
 * @param error optional, if provided and an error occurs the contents are replaced with an error message
 * @returns as per fread or read, positive values indicate bytes read, 0 indicates no bytes available, negative values indicate errors
 */
int stream_read(stream *stream, void *dst, size_t n, string *error);

/**
 * @param src the source of bytes to write to the stream
 * @param n the number of bytes to write
 * @param error optional, if provided and an error occurs the contents are replaced with an error message
 * @returns as per fwrite or write, positive values indicate bytes written, negative values indiciate errors
 */
int stream_write(stream *stream, void *src, size_t n, string *error);

/**
 * As stream_read, but reads into the given buffer. Expands the buffer as needed to fill in new data.
 */
int stream_read_buffer(stream *stream, buffer *dst, size_t n, string *error);

#ifdef __cplusplus
}
#endif

#endif