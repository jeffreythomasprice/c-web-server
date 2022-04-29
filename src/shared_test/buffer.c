#include <assert.h>

#include "../shared/buffer.h"

int main() {
	buffer b;

	buffer_init(&b);
	assert(b.capacity == 0);
	assert(b.length == 0);
	assert(b.data == NULL);

	buffer_set_capacity(&b, 5);
	assert(b.capacity == 5);
	assert(b.length == 0);
	assert(b.data != NULL);

	buffer_set_capacity(&b, 10);
	assert(b.capacity == 10);
	assert(b.length == 0);
	assert(b.data != NULL);

	buffer_set_length(&b, 10);
	assert(b.capacity == 10);
	assert(b.length == 10);
	assert(b.data != NULL);

	buffer_set_length(&b, 15);
	assert(b.capacity == 15);
	assert(b.length == 15);
	assert(b.data != NULL);

	buffer_set_length(&b, 5);
	assert(b.capacity == 15);
	assert(b.length == 5);
	assert(b.data != NULL);

	buffer_set_capacity(&b, 2);
	assert(b.capacity == 2);
	assert(b.length == 2);
	assert(b.data != NULL);

	buffer_set_capacity(&b, 0);
	assert(b.capacity == 0);
	assert(b.length == 0);
	assert(b.data == NULL);

	buffer_dealloc(&b);

	buffer_init_copy(&b, "abcd", 4);
	assert(b.capacity == 4);
	assert(b.length == 4);
	assert(!memcmp(b.data, "abcd", 4));

	buffer_set_length(&b, 3);
	assert(b.capacity == 4);
	assert(b.length == 3);
	assert(!memcmp(b.data, "abc", 3));

	buffer_append_bytes(&b, "xyz", 3);
	assert(b.capacity == 6);
	assert(b.length == 6);
	assert(!memcmp(b.data, "abcxyz", 6));

	buffer_dealloc(&b);

	return 0;
}