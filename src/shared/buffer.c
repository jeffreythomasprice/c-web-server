#include <string.h>

#include "buffer.h"

void buffer_init(buffer *b) {
	b->capacity = 0;
	b->length = 0;
	b->data = NULL;
}

void buffer_init_copy(buffer *b, uint8_t *data, size_t len) {
	b->capacity = len;
	b->length = len;
	b->data = malloc(len);
	memcpy(b->data, data, len);
}

void buffer_dealloc(buffer *b) {
	if (b->data) {
		free(b->data);
	}
}

size_t buffer_get_capacity(buffer *b) {
	return b->capacity;
}

void buffer_set_capacity(buffer *b, size_t new_capacity) {
	if (new_capacity != b->capacity) {
		if (new_capacity == 0) {
			free(b->data);
			b->data = NULL;
		} else {
			b->data = realloc(b->data, new_capacity);
		}
		b->capacity = new_capacity;
		if (b->capacity < b->length) {
			b->length = b->capacity;
		}
	}
}

void buffer_ensure_capacity(buffer *b, size_t new_capacity) {
	if (new_capacity > b->capacity) {
		buffer_set_capacity(b, new_capacity);
	}
}

size_t buffer_get_length(buffer *b) {
	return b->length;
}

void buffer_set_length(buffer *b, size_t new_length) {
	if (b->length != new_length) {
		buffer_ensure_capacity(b, new_length);
		b->length = new_length;
	}
}

void buffer_clear(buffer *b) {
	buffer_set_length(b, 0);
}

void buffer_append_bytes(buffer *b, void *data, size_t len) {
	size_t desired_length = b->length + len;
	size_t desired_index = b->length;
	buffer_set_length(b, desired_length);
	memcpy(b->data + desired_index, data, len);
}
