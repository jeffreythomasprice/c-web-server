#ifndef buffer_h
#define buffer_h

#ifdef __cplusplus
extern "C" {
#endif

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

size_t buffer_get_capacity(buffer *b);
/**
 * Resizes the internal storage to exactly this many bytes. Existing data is preserved (i.e. realloc). If the new capcity is smaller than
 * existing data is lost at the end. If the new capacity is larger than the current length the current length is adjusted to be the entire
 * remaining data.
 */
void buffer_set_capacity(buffer *b, size_t new_capacity);
/**
 * If the new capcity is larger than the current, sets capacity to the new capacity.
 */
void buffer_ensure_capacity(buffer *b, size_t new_capacity);
size_t buffer_get_length(buffer *b);
/**
 * Sets the length to exactly this many bytes. If the new value is larger than the current capacity the underlying data is resized to fit
 * (realloc).
 */
void buffer_set_length(buffer *b, size_t new_length);

/**
 * Sets length to 0.
 */
void buffer_clear(buffer *b);

void buffer_append_bytes(buffer *b, void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif