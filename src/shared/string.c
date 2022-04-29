#include <string.h>

#include "string.h"

void string_init(string *s) {
	buffer_init(&s->b);
	buffer_set_length(&s->b, 1);
	s->b.data[0] = 0;
}

void string_init_cstr(string *s, char *c) { string_init_cstr_len(s, c, strlen(c)); }

void string_init_cstr_len(string *s, char *c, size_t len) {
	buffer_init(&s->b);
	buffer_set_capacity(&s->b, len + 1);
	buffer_append_bytes(&s->b, c, len);
	buffer_set_length(&s->b, len + 1);
	s->b.data[len] = 0;
}

void string_dealloc(string *s) { buffer_dealloc(&s->b); }

size_t string_get_length(string *s) {
	// -1 because the length of the buffer includes the terminating 0
	return s->b.length - 1;
}

char *string_get_cstr(string *s) { return s->b.data; }
