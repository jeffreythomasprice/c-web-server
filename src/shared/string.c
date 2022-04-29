#include <stdarg.h>
#include <stdio.h>
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

void string_clear(string *s) {
	buffer_set_length(&s->b, 1);
	s->b.data[0] = 0;
}

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

void string_append_substr(string *dst, string *src, size_t start, size_t end) {
	size_t src_len = string_get_length(src);
	if (start >= end || start >= src_len) {
		return;
	}
	if (end > src_len) {
		end = src_len;
	}
	size_t substr_len = end - start;
	size_t dst_len = string_get_length(dst);
	// resize to fit
	buffer_set_capacity(&dst->b, dst_len + substr_len + 1);
	// chop off the trailing 0 from this string
	buffer_set_length(&dst->b, dst_len);
	// write the substring here
	buffer_append_bytes(&dst->b, src->b.data + start, substr_len);
	// write the trailing 0
	buffer_set_length(&dst->b, dst_len + substr_len + 1);
	dst->b.data[dst_len + substr_len] = 0;
}

size_t string_split(string *s, char *delim, size_t *results, size_t results_capacity, size_t max_results) {
	if (max_results == 0) {
		max_results = -1;
	}
	size_t len = string_get_length(s);
	size_t delim_len = strlen(delim);
	if (delim_len > len) {
		if (results_capacity >= 1) {
			results[0] = 0;
			results[1] = len;
		}
		return 1;
	}
	size_t result_count = 0;
	size_t start = 0;
	for (size_t i = 0; i < len - delim_len; i++) {
		if (!memcmp(s->b.data + i, delim, delim_len)) {
			results[result_count * 2 + 0] = start;
			results[result_count * 2 + 1] = i;
			start = i + delim_len;
			i += delim_len - 1;
			result_count++;
			if (result_count + 1 >= max_results) {
				break;
			}
		}
	}
	if (start < len) {
		if (result_count < max_results) {
			results[result_count * 2 + 0] = start;
			results[result_count * 2 + 1] = len;
		}
		result_count++;
	}
	return result_count;
}