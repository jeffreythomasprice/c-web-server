#include <stdarg.h>
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

void string_appends(string *s, string *other) {
	// resize to fit both
	buffer_set_capacity(&s->b, string_get_length(s) + string_get_length(other) + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&s->b, string_get_length(s));
	// add the other one, including that trailing 0
	buffer_append_bytes(&s->b, other->b.data, string_get_length(other) + 1);
}

void string_appendf(string *s, char *fmt, ...) {
	// check how much more space we need
	va_list args;
	va_start(args, fmt);
	size_t n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	// resize to fit
	size_t current_length = string_get_length(s);
	buffer_set_capacity(&s->b, current_length + n + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&s->b, current_length);
	// write the other one here
	va_start(args, fmt);
	vsnprintf(s->b.data + current_length, n + 1, fmt, args);
	va_end(args);
}