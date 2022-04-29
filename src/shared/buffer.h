#ifndef buffer_h
#define buffer_h

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	size_t capacity;
	size_t length;
	uint8_t *data;
} buffer;

void buffer_init(buffer *b);
void buffer_init_copy(buffer *b, uint8_t *data, size_t len);
void buffer_dealloc(buffer *b);

void buffer_set_capacity(buffer *b, size_t new_capacity);
void buffer_set_length(buffer *b, size_t new_length);

void buffer_append_bytes(buffer *b, void *data, size_t len);

#endif